package dev.edxeme.lmh;

import java.util.List;

record CaseResult(String name, String status, double ms, String detail) {
    boolean pass() {
        return "pass".equals(status);
    }
}

/**
 * lmh.report/v1 — the same shape every implementation emits.
 */
final class Report {

    final String task;
    final String impl;
    final String source;
    final double durationMs;
    final List<CaseResult> cases;
    final int total;
    final int passed;
    final String buildError;

    Report(String task, String impl, String source, double durationMs, List<CaseResult> cases,
            int total, int passed, String buildError) {
        this.task = task;
        this.impl = impl;
        this.source = source;
        this.durationMs = durationMs;
        this.cases = cases;
        this.total = total;
        this.passed = passed;
        this.buildError = buildError;
    }

    int failed() {
        return total - passed;
    }

    double score() {
        return total == 0 ? 0.0 : (double) passed / total;
    }

    String toJson() {
        StringBuilder sb = new StringBuilder();
        sb.append("{\"schema\":\"lmh.report/v1\",\"task\":\"").append(MiniJson.escape(task))
                .append("\",\"impl\":\"").append(impl)
                .append("\",\"source\":\"").append(MiniJson.escape(source))
                .append("\",\"duration_ms\":").append(String.format(java.util.Locale.ROOT, "%.3f", durationMs))
                .append(",\"summary\":{\"total\":").append(total)
                .append(",\"passed\":").append(passed)
                .append(",\"failed\":").append(failed())
                .append("},\"score\":").append(String.format(java.util.Locale.ROOT, "%.3f", score()))
                .append(",\"cases\":[");
        for (int i = 0; i < cases.size(); i++) {
            CaseResult c = cases.get(i);
            if (i > 0) {
                sb.append(',');
            }
            sb.append("{\"name\":\"").append(MiniJson.escape(c.name()))
                    .append("\",\"status\":\"").append(c.status())
                    .append("\",\"ms\":").append(String.format(java.util.Locale.ROOT, "%.3f", c.ms()))
                    .append(",\"detail\":\"").append(MiniJson.escape(c.detail()))
                    .append("\"}");
        }
        sb.append(']');
        if (buildError != null) {
            sb.append(",\"build_error\":\"").append(MiniJson.escape(buildError)).append('"');
        }
        return sb.append('}').toString();
    }

    String toHuman() {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("task %s · impl %s · source %s%n", task, impl, source));
        for (CaseResult c : cases) {
            sb.append(String.format("  %-4s %-28s %8.3fms", c.pass() ? "pass" : "FAIL", c.name(), c.ms()));
            if (!c.pass() && !c.detail().isEmpty()) {
                sb.append("  ").append(c.detail());
            }
            sb.append('\n');
        }
        if (buildError != null) {
            sb.append("  build error: ").append(buildError).append('\n');
        }
        sb.append(String.format("%d/%d passed · score %.3f · %.1fms%n", passed, total, score(), durationMs));
        return sb.toString();
    }
}
