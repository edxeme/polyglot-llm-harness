use std::path::{Path, PathBuf};

pub struct TaskSpec {
    pub id: &'static str,
    pub candidate_file_name: &'static str,
    pub cases_path: &'static str,
    /// Embedded at build time; used when no --candidate is supplied.
    pub golden: &'static str,
}

impl TaskSpec {
    /// Candidate file for a fresh temp dir: either the golden or the
    /// user-supplied candidate, always copied under the canonical name so
    /// the generated driver can reference it as `mod candidate;`.
    pub fn source(&self, candidate: Option<&Path>) -> Result<String, String> {
        match candidate {
            None => Ok(self.golden.to_string()),
            Some(path) => std::fs::read_to_string(path).map_err(|e| format!("cannot read {}: {e}", path.display())),
        }
    }

    pub fn cases(&self, root: &Path) -> Result<String, String> {
        let path: PathBuf = root.join(self.cases_path);
        std::fs::read_to_string(&path).map_err(|e| format!("cannot read {}: {e}", path.display()))
    }
}

macro_rules! golden {
    ($file:literal) => {
        include_str!(concat!(env!("CARGO_MANIFEST_DIR"), "/golden/", $file))
    };
}

pub const ALL: &[TaskSpec] = &[TaskSpec {
    id: "merge-intervals",
    candidate_file_name: "candidate.rs",
    cases_path: "rust/verifier/cases/merge-intervals.cases",
    golden: golden!("merge_intervals.rs"),
}];

pub fn find(id: &str) -> Option<&'static TaskSpec> {
    ALL.iter().find(|t| t.id == id)
}
