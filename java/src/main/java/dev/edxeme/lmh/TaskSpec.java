package dev.edxeme.lmh;

/**
 * A graded coding task: a statement in tasks/, a bundled golden solution and a
 * case table that the harness compiles into a generated verifier.
 */
record TaskSpec(String id, String candidateFileName, String casesPath) {}
