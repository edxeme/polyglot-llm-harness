mod json;
mod registry;
mod report;
mod runner;

use std::process::ExitCode;

const USAGE: &str = "lmh — polyglot llm verifier harness (rust implementation)\n\nusage:\n  lmh tasks\n  lmh run <task> [--candidate <file>] [--json]\n\nexit codes: 0 pass, 1 case failures, 2 build failure, 3 usage";

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().skip(1).collect();
    match run(&args) {
        Ok(code) => ExitCode::from(code as u8),
        Err(e) => {
            eprintln!("lmh: {e}");
            ExitCode::from(3)
        }
    }
}

fn run(args: &[String]) -> Result<u32, String> {
    match args.first().map(String::as_str) {
        None | Some("-h" | "--help" | "help") => {
            println!("{USAGE}");
            Ok(if args.is_empty() { 3 } else { 0 })
        }
        Some("tasks") => {
            for t in registry::ALL {
                println!("{:<16} candidate {}", t.id, t.candidate_file_name);
            }
            Ok(0)
        }
        Some("run") => runner::run_task(args),
        Some(other) => Err(format!("unknown command: {other}")),
    }
}
