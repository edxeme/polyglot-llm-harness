//! lmh.report/v1 — the same shape every implementation emits.

pub struct CaseResult {
    pub name: String,
    pub status: String,
    pub ms: f64,
    pub detail: String,
}

impl CaseResult {
    fn pass(&self) -> bool {
        self.status == "pass"
    }

    fn to_json(&self) -> String {
        format!(
            "{{\"name\":\"{}\",\"status\":\"{}\",\"ms\":{:.3},\"detail\":\"{}\"}}",
            escape(&self.name),
            self.status,
            self.ms,
            escape(&self.detail)
        )
    }
}

pub struct Report {
    pub task: String,
    pub source: String,
    pub duration_ms: f64,
    pub cases: Vec<CaseResult>,
    pub total: u32,
    pub passed: u32,
    pub build_error: Option<String>,
}

fn escape(s: &str) -> String {
    let mut out = String::with_capacity(s.len() + 8);
    for c in s.chars() {
        match c {
            '\\' => out.push_str("\\\\"),
            '"' => out.push_str("\\\""),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            c if (c as u32) < 0x20 => out.push_str(&format!("\\u{:04x}", c as u32)),
            c => out.push(c),
        }
    }
    out
}

impl Report {
    pub fn failed(&self) -> u32 {
        self.total - self.passed
    }

    pub fn score(&self) -> f64 {
        if self.total == 0 {
            0.0
        } else {
            self.passed as f64 / self.total as f64
        }
    }

    pub fn to_json(&self) -> String {
        let mut out = String::new();
        out.push_str(&format!(
            "{{\"schema\":\"lmh.report/v1\",\"task\":\"{}\",\"impl\":\"rust\",\"source\":\"{}\",\"duration_ms\":{:.3},\"summary\":{{\"total\":{},\"passed\":{},\"failed\":{}}},\"score\":{:.3},\"cases\":[",
            escape(&self.task),
            escape(&self.source),
            self.duration_ms,
            self.total,
            self.passed,
            self.failed(),
            self.score()
        ));
        for (i, c) in self.cases.iter().enumerate() {
            if i > 0 {
                out.push(',');
            }
            out.push_str(&c.to_json());
        }
        out.push(']');
        if let Some(e) = &self.build_error {
            out.push_str(&format!(",\"build_error\":\"{}\"", escape(e)));
        }
        out.push('}');
        out
    }

    pub fn to_human(&self) -> String {
        let mut out = format!("task {} · impl rust · source {}\n", self.task, self.source);
        for c in &self.cases {
            out.push_str(&format!(
                "  {:<4} {:<28} {:>8.3}ms",
                if c.pass() { "pass" } else { "FAIL" },
                c.name,
                c.ms
            ));
            if !c.pass() && !c.detail.is_empty() {
                out.push_str("  ");
                out.push_str(&c.detail);
            }
            out.push('\n');
        }
        if let Some(e) = &self.build_error {
            out.push_str(&format!("  build error: {e}\n"));
        }
        out.push_str(&format!(
            "{}/{} passed · score {:.3} · {:.1}ms\n",
            self.passed,
            self.total,
            self.score(),
            self.duration_ms
        ));
        out
    }
}
