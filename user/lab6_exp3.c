#include <stdbool.h>
#include <stdio.h>

#define MAX_FRAMES 10

static bool contains(int frames[], int frame_count, int page, int *pos_out) {
    for (int i = 0; i < frame_count; i++) {
        if (frames[i] == page) {
            if (pos_out) {
                *pos_out = i;
            }
            return true;
        }
    }
    return false;
}

static int fifo_algorithm(int frame_count, const int *sequence, int length) {
    int frames[MAX_FRAMES];
    for (int i = 0; i < frame_count; i++) {
        frames[i] = -1;
    }
    int front = 0;
    int size = 0;
    int faults = 0;
    for (int i = 0; i < length; i++) {
        int page = sequence[i];
        if (contains(frames, frame_count, page, NULL)) {
            continue;
        }
        faults++;
        if (size < frame_count) {
            frames[size++] = page;
        } else {
            frames[front] = page;
            front = (front + 1) % frame_count;
        }
    }
    return faults;
}

static int lru_algorithm(int frame_count, const int *sequence, int length) {
    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES];
    for (int i = 0; i < frame_count; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }
    int faults = 0;
    for (int t = 0; t < length; t++) {
        int page = sequence[t];
        int pos = -1;
        if (contains(frames, frame_count, page, &pos)) {
            last_used[pos] = t;
            continue;
        }
        faults++;
        int target = -1;
        for (int i = 0; i < frame_count; i++) {
            if (frames[i] == -1) {
                target = i;
                break;
            }
        }
        if (target == -1) {
            int oldest_time = last_used[0];
            target = 0;
            for (int i = 1; i < frame_count; i++) {
                if (last_used[i] < oldest_time) {
                    oldest_time = last_used[i];
                    target = i;
                }
            }
        }
        frames[target] = page;
        last_used[target] = t;
    }
    return faults;
}

static int optimal_algorithm(int frame_count, const int *sequence, int length) {
    int frames[MAX_FRAMES];
    for (int i = 0; i < frame_count; i++) {
        frames[i] = -1;
    }
    int faults = 0;
    for (int i = 0; i < length; i++) {
        int page = sequence[i];
        if (contains(frames, frame_count, page, NULL)) {
            continue;
        }
        faults++;
        int target = -1;
        for (int f = 0; f < frame_count; f++) {
            if (frames[f] == -1) {
                target = f;
                break;
            }
        }
        if (target == -1) {
            int farthest_idx = -1;
            int farthest_frame = 0;
            for (int f = 0; f < frame_count; f++) {
                int next_use = length + 1; // 视为“永不再用”
                for (int j = i + 1; j < length; j++) {
                    if (sequence[j] == frames[f]) {
                        next_use = j;
                        break;
                    }
                }
                if (next_use > farthest_idx) {
                    farthest_idx = next_use;
                    farthest_frame = f;
                }
            }
            target = farthest_frame;
        }
        frames[target] = page;
    }
    return faults;
}

static void run_for_frames(int frame_count, const int *sequence, int length) {
    int fifo_faults = fifo_algorithm(frame_count, sequence, length);
    int lru_faults = lru_algorithm(frame_count, sequence, length);
    int opt_faults = optimal_algorithm(frame_count, sequence, length);
    double fifo_rate = (double)fifo_faults / length;
    double lru_rate = (double)lru_faults / length;
    double opt_rate = (double)opt_faults / length;
    printf("Frames=%d | FIFO: %2d (%.2f) | LRU: %2d (%.2f) | OPT: %2d (%.2f)\n",
           frame_count,
           fifo_faults, fifo_rate,
           lru_faults, lru_rate,
           opt_faults, opt_rate);
}

int main(void) {
    const int sequence[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    const int length = sizeof(sequence) / sizeof(sequence[0]);
    const int frame_options[] = {3, 4, 5};

    printf("=== Page replacement comparison (sequence: {1 2 3 4 1 2 5 1 2 3 4 5}) ===\n");
    for (int i = 0; i < 3; i++) {
        run_for_frames(frame_options[i], sequence, length);
    }
    printf("\nBelady anomaly check: compare FIFO fault counts as frames increase.\n");
    return 0;
}
