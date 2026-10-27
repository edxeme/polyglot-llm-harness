import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class MergeIntervals {

    private MergeIntervals() {}

    public static int[][] merge(int[][] intervals) {
        for (int[] iv : intervals) {
            if (iv[0] > iv[1]) {
                throw new IllegalArgumentException("invalid interval: [" + iv[0] + "," + iv[1] + "]");
            }
        }
        int[][] sorted = intervals.clone();
        Arrays.sort(sorted, (a, b) -> a[0] != b[0]
                ? Integer.compare(a[0], b[0])
                : Integer.compare(a[1], b[1]));
        List<int[]> merged = new ArrayList<>();
        for (int[] iv : sorted) {
            if (!merged.isEmpty() && iv[0] <= merged.get(merged.size() - 1)[1]) {
                int[] last = merged.get(merged.size() - 1);
                last[1] = Math.max(last[1], iv[1]);
            } else {
                merged.add(new int[] {iv[0], iv[1]});
            }
        }
        return merged.toArray(new int[0][]);
    }
}
