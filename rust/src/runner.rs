//! Grades one candidate (or the embedded golden) for a task: compose a
//! verifier program in a temp dir, build it with rustc, run it, parse the
//! case lines and emit a report. The candidate is never loaded into this
//! process.

use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::Instant;

use crate::json::{parse_object, Value};
use crate::registry::{self, TaskSpec};
use crate::report::{CaseResult, Report};

pub const EXIT_PASS: u32 = 0;
pub const EXIT_CASE_FAILURES: u32 = 1;
pub const EXIT_BUILD_FAILURE: u32 = 2;

pub fn run_task(args: &[String]) -> Result<u32, String> {
    if args.len() < 2 {
        return Err("run requires a task id (see 'lmh tasks')".into());
    }
    let mut candidate: Option<PathBuf> = None;
    let mut json = false;
    let mut rest = &args[2..];
    while !rest.is_empty() {
        match rest[0].as_str() {
            "--candidate" => {
                if rest.len() < 2 {
                    return Err("--candidate requires a path".into());
                }
                candidate = Some(PathBuf::from(&rest[1]));
                rest = &rest[2..];
            }
            "--json" => {
                json = true;
                rest = &rest[1..];
            }
            other => return Err(format!("unknown option: {other}")),
        }
    }
    let spec = registry::find(&args[1]).ok_or_else(|| format!("unknown task: {} (see 'lmh tasks')", args[1]))?;

    let code = run_spec(spec, candidate.as_deref(), json)?;
    Ok(code)
}

fn run_spec(spec: &TaskSpec, candidate: Option<&Path>, json: bool) -> Result<u32, String> {
    let t0 = Instant::now();
    let root = crate::runner::find_root()?;
    let source = spec.source(candidate)?;
    let cases = spec.cases(&root)?;

    let tmp = mktemp_dir()?;
    let result = drive(spec, &source, &cases, &root, &tmp);
    let _ = std::fs::remove_dir_all(&tmp);

    let (cases_out, total, passed, build_error) = match result {
        Ok(v) => v,
        Err(msg) => {
            let report = Report {
                task: spec.id.to_string(),
                source: source_label(candidate, &source),
                duration_ms: t0.elapsed().as_secs_f64() * 1000.0,
                cases: Vec::new(),
                total: 0,
                passed: 0,
                build_error: Some(msg.clone()),
            };
            println!("{}", if json { report.to_json() } else { report.to_human() });
            return Ok(EXIT_BUILD_FAILURE);
        }
    };

    let report = Report {
        task: spec.id.to_string(),
        source: source_label(candidate, &source),
        duration_ms: t0.elapsed().as_secs_f64() * 1000.0,
        cases: cases_out,
        total,
        passed,
        build_error,
    };
    println!("{}", if json { report.to_json() } else { report.to_human() });
    if report.build_error.is_some() {
        return Ok(EXIT_BUILD_FAILURE);
    }
    Ok(if report.failed() > 0 { EXIT_CASE_FAILURES } else { EXIT_PASS })
}

type DriveOut = (Vec<CaseResult>, u32, u32, Option<String>);

fn drive(spec: &TaskSpec, source: &str, cases: &str, root: &Path, tmp: &Path) -> Result<DriveOut, String> {
    let template = read_template(root, &spec.cases_path)?;
    let driver = template.replace("// CASES", cases.trim());
    std::fs::write(tmp.join("candidate.rs"), source).map_err(|e| e.to_string())?;
    std::fs::write(tmp.join("main.rs"), driver).map_err(|e| e.to_string())?;

    let build = Command::new("rustc")
        .args(["--edition", "2021", "-O", "main.rs", "-o", "verifier"])
        .current_dir(tmp)
        .output()
        .map_err(|e| format!("cannot run rustc: {e}"))?;
    if !build.status.success() {
        return Err(first_line(&String::from_utf8_lossy(&build.stderr)).unwrap_or_else(|| "rustc failed".into()));
    }

    let run = Command::new(tmp.join("verifier"))
        .current_dir(tmp)
        .output()
        .map_err(|e| format!("cannot run verifier: {e}"))?;
    let stdout = String::from_utf8_lossy(&run.stdout);
    let mut case_results = Vec::new();
    let (mut total, mut passed) = (0u32, 0u32);
    for line in stdout.lines() {
        let line = line.trim();
        if !line.starts_with('{') {
            continue;
        }
        let obj = match parse_object(line) {
            Ok(o) => o,
            Err(_) => continue,
        };
        match obj.get("summary") {
            Some(Value::Obj(s)) => {
                total = num(s.get("total")) as u32;
                passed = num(s.get("passed")) as u32;
            }
            _ => {
                if let Some(Value::Str(name)) = obj.get("case") {
                    let status = match obj.get("status") {
                        Some(Value::Str(s)) => s.clone(),
                        _ => "fail".into(),
                    };
                    let ms = num(obj.get("ms"));
                    let detail = match obj.get("detail") {
                        Some(Value::Str(d)) => d.clone(),
                        _ => String::new(),
                    };
                    case_results.push(CaseResult { name: name.clone(), status, ms, detail });
                }
            }
        }
    }
    let build_error = match run.status.code() {
        Some(0) | Some(1) => None,
        code => Some(format!("verifier exited with {:?}", code)),
    };
    Ok((case_results, total, passed, build_error))
}

fn read_template(root: &Path, cases_path: &str) -> Result<String, String> {
    // cases live under rust/verifier/cases/*.cases; the driver sits one level up
    let path = PathBuf::from(cases_path)
        .parent()
        .and_then(|p| p.parent())
        .map(|p| root.join(p).join("driver.rs.template"))
        .ok_or_else(|| "bad cases path".to_string())?;
    std::fs::read_to_string(&path).map_err(|e| format!("cannot read {}: {e}", path.display()))
}

fn source_label(candidate: Option<&Path>, _source: &str) -> String {
    match candidate {
        Some(p) => p.display().to_string(),
        None => "golden".into(),
    }
}

fn num(v: Option<&Value>) -> f64 {
    match v {
        Some(Value::Num(n)) => *n,
        _ => 0.0,
    }
}

fn first_line(s: &str) -> Option<String> {
    s.lines().map(str::trim).find(|l| !l.is_empty()).map(String::from)
}

fn mktemp_dir() -> Result<PathBuf, String> {
    let base = std::env::temp_dir();
    let nanos = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos();
    let dir = base.join(format!("lmh-{}-{}", std::process::id(), nanos));
    std::fs::create_dir_all(&dir).map_err(|e| e.to_string())?;
    Ok(dir)
}

pub fn find_root() -> Result<PathBuf, String> {
    let mut dir = std::env::current_dir().map_err(|e| e.to_string())?;
    loop {
        if dir.join("tasks").is_dir() && dir.join("docs/spec.md").is_file() {
            return Ok(dir);
        }
        if !dir.pop() {
            return Err("not inside an lmh checkout (no tasks/ and docs/spec.md upward)".into());
        }
    }
}
