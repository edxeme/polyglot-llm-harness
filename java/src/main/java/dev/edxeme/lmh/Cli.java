package dev.edxeme.lmh;

import java.io.PrintStream;
import java.nio.file.Path;

final class Cli {

    private Cli() {}

    static int run(String[] args) {
        if (args.length == 0) {
            usage(System.err);
            return 3;
        }
        return switch (args[0]) {
            case "tasks" -> {
                for (TaskSpec t : TaskRegistry.ALL) {
                    System.out.printf("%-16s candidate %s%n", t.id(), t.candidateFileName());
                }
                yield 0;
            }
            case "run" -> runTask(args);
            case "-h", "--help", "help" -> {
                usage(System.out);
                yield 0;
            }
            default -> {
                System.err.println("lmh: unknown command: " + args[0]);
                yield 3;
            }
        };
    }

    private static int runTask(String[] args) {
        if (args.length < 2) {
            System.err.println("lmh: run requires a task id (see 'lmh tasks')");
            return 3;
        }
        Path candidate = null;
        boolean json = false;
        for (int i = 2; i < args.length; i++) {
            switch (args[i]) {
                case "--candidate" -> {
                    if (i + 1 >= args.length) {
                        System.err.println("lmh: --candidate requires a path");
                        return 3;
                    }
                    candidate = Path.of(args[++i]);
                }
                case "--json" -> json = true;
                default -> {
                    System.err.println("lmh: unknown option: " + args[i]);
                    return 3;
                }
            }
        }
        TaskSpec spec = TaskRegistry.find(args[1]);
        if (spec == null) {
            System.err.println("lmh: unknown task: " + args[1] + " (see 'lmh tasks')");
            return 3;
        }
        try {
            return Runner.run(spec, candidate, json, System.out, System.err);
        } catch (Exception e) {
            System.err.println("lmh: " + e.getMessage());
            return 2;
        }
    }

    private static void usage(PrintStream out) {
        out.print("""
                lmh — polyglot llm verifier harness (java implementation)

                usage:
                  lmh tasks
                  lmh run <task> [--candidate <file>] [--json]

                exit codes: 0 pass, 1 case failures, 2 build failure, 3 usage
                """);
    }
}
