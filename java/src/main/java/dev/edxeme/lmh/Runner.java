package dev.edxeme.lmh;

import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Grades one candidate (or the bundled golden solution) for a task:
 * compose a verifier program in a temp dir, build it with javac, run it,
 * parse the case lines and emit a report. The candidate is never loaded
 * into this process.
 */
final class Runner {

    static final int EXIT_PASS = 0;
    static final int EXIT_CASE_FAILURES = 1;
    static final int EXIT_BUILD_FAILURE = 2;

    private Runner() {}

    static int run(TaskSpec spec, Path candidate, boolean json, PrintStream out, PrintStream err)
            throws IOException, InterruptedException {
        long t0 = System.nanoTime();
        Path root = RepoRoot.find();
        Path source = candidate != null
                ? candidate.toAbsolutePath()
                : root.resolve("java").resolve("golden").resolve(spec.candidateFileName());
        if (!Files.isRegularFile(source)) {
            err.println("lmh: source not found: " + source);
            return 3;
        }

        String template = Files.readString(
                root.resolve("java").resolve("verifier").resolve("driver.java.template"), StandardCharsets.UTF_8);
        String cases = Files.readString(root.resolve(spec.casesPath()), StandardCharsets.UTF_8);
        String verifierSource = template.replace("// CASES", cases.strip());

        Path tmp = Files.createTempDirectory("lmh-");
        try {
            Path classes = tmp.resolve("classes");
            Files.createDirectories(classes);
            Path primary = tmp.resolve(spec.candidateFileName());
            Files.copy(source, primary, StandardCopyOption.REPLACE_EXISTING);
            Path verifierJava = tmp.resolve("LMHVerifier.java");
            Files.writeString(verifierJava, verifierSource, StandardCharsets.UTF_8);

            Exec build = execMerged(
                    List.of("javac", "-d", classes.toString(), primary.toString(), verifierJava.toString()), 120);
            if (build.exit != 0) {
                return finish(spec, sourceLabel(candidate, source), t0, List.of(), 0, 0,
                        firstLine(build.output, "javac failed"), json, out, err);
            }

            Path stderrFile = tmp.resolve("verifier.stderr");
            Process proc = new ProcessBuilder("java", "-cp", classes.toString(), "LMHVerifier")
                    .redirectError(stderrFile.toFile())
                    .start();
            String stdout;
            try (var is = proc.getInputStream()) {
                stdout = new String(is.readAllBytes(), StandardCharsets.UTF_8);
            }
            if (!proc.waitFor(60, TimeUnit.SECONDS)) {
                proc.destroyForcibly();
                return finish(spec, sourceLabel(candidate, source), t0, List.of(), 0, 0,
                        "verifier timed out", json, out, err);
            }
            String stderr = Files.readString(stderrFile, StandardCharsets.UTF_8);

            List<CaseResult> caseResults = new ArrayList<>();
            int total = 0;
            int passed = 0;
            for (String line : stdout.split("\n")) {
                String trimmed = line.trim();
                if (!trimmed.startsWith("{")) {
                    continue;
                }
                Map<String, Object> obj;
                try {
                    obj = MiniJson.parseObject(trimmed);
                } catch (RuntimeException ignore) {
                    continue;
                }
                if (obj.get("summary") instanceof Map<?, ?> s) {
                    total = (int) Math.round(asDouble(s.get("total")));
                    passed = (int) Math.round(asDouble(s.get("passed")));
                    continue;
                }
                if (obj.get("case") instanceof String name) {
                    caseResults.add(new CaseResult(
                            name,
                            String.valueOf(obj.getOrDefault("status", "fail")),
                            obj.get("ms") instanceof Double d ? d : 0.0,
                            obj.get("detail") instanceof String d ? d : ""));
                }
            }
            String buildError = switch (proc.exitValue()) {
                case 0, 1 -> null;
                default -> "verifier exited with " + proc.exitValue() + firstLine(stderr, "").trim();
            };
            return finish(spec, sourceLabel(candidate, source), t0, caseResults, total, passed,
                    buildError, json, out, err);
        } finally {
            deleteRecursively(tmp);
        }
    }

    private static int finish(TaskSpec spec, String source, long t0, List<CaseResult> cases,
            int total, int passed, String buildError, boolean json, PrintStream out, PrintStream err) {
        double durationMs = (System.nanoTime() - t0) / 1_000_000.0;
        Report report = new Report(spec.id(), "java", source, durationMs, cases, total, passed, buildError);
        out.println(json ? report.toJson() : report.toHuman());
        if (buildError != null) {
            return EXIT_BUILD_FAILURE;
        }
        return report.failed() > 0 ? EXIT_CASE_FAILURES : EXIT_PASS;
    }

    private static String sourceLabel(Path candidate, Path source) {
        return candidate != null ? source.toString() : "golden";
    }

    private static double asDouble(Object o) {
        return o instanceof Double d ? d : Double.parseDouble(String.valueOf(o));
    }

    private static String firstLine(String s, String fallback) {
        for (String line : s.split("\n")) {
            if (!line.isBlank()) {
                return line.strip();
            }
        }
        return fallback;
    }

    private static void deleteRecursively(Path dir) throws IOException {
        if (!Files.exists(dir)) {
            return;
        }
        try (var walk = Files.walk(dir)) {
            walk.sorted(Comparator.reverseOrder()).forEach(p -> {
                try {
                    Files.delete(p);
                } catch (IOException ignored) {
                    // best effort cleanup
                }
            });
        }
    }

    private record Exec(int exit, String output) {}

    private static Exec execMerged(List<String> cmd, long timeoutSec) throws IOException, InterruptedException {
        Process p = new ProcessBuilder(cmd).redirectErrorStream(true).start();
        String output;
        try (var is = p.getInputStream()) {
            output = new String(is.readAllBytes(), StandardCharsets.UTF_8);
        }
        if (!p.waitFor(timeoutSec, TimeUnit.SECONDS)) {
            p.destroyForcibly();
            return new Exec(-1, output + "\n[timeout]");
        }
        return new Exec(p.exitValue(), output);
    }
}
