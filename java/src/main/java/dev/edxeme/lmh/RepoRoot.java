package dev.edxeme.lmh;

import java.nio.file.Path;
import java.nio.file.Paths;

final class RepoRoot {

    private RepoRoot() {}

    static Path find() {
        Path dir = Paths.get("").toAbsolutePath();
        while (dir != null) {
            if (dir.resolve("tasks").toFile().isDirectory()
                    && dir.resolve("docs").resolve("spec.md").toFile().isFile()) {
                return dir;
            }
            dir = dir.getParent();
        }
        throw new IllegalStateException("not inside an lmh checkout (no tasks/ and docs/spec.md upward)");
    }
}
