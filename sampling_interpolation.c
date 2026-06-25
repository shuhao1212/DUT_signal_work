#include <stdio.h>
#include <stdlib.h>

/* Downsample function: g_M(n) = x(M * n) */
int* downsample(const int* x, int x_len, int offset, int M,
                int** out, int* out_len, int* out_offset) {
    int n_min = (0 - offset) / M;
    if ((0 - offset) % M != 0 && (0 - offset) < 0) n_min--;
    int n_max = (x_len - 1 - offset) / M;
    if ((x_len - 1 - offset) % M != 0 && (x_len - 1 - offset) < 0) n_max--;

    int len = n_max - n_min + 1;
    if (len <= 0) {
        *out = NULL;
        *out_len = 0;
        *out_offset = 0;
        return NULL;
    }

    int* result = (int*)malloc(len * sizeof(int));
    if (!result) {
        *out = NULL;
        return NULL;
    }

    for (int n = n_min; n <= n_max; n++) {
        int idx = offset + M * n;
        result[n - n_min] = x[idx];
    }

    *out = result;
    *out_len = len;
    *out_offset = -n_min;
    return result;
}

/* Interpolate function (zero insertion): f_K(n) = x(n/K) if n%K==0 else 0 */
int* interpolate(const int* x, int x_len, int offset, int K,
                 int** out, int* out_len, int* out_offset) {
    int n_orig_min = -offset;
    int n_orig_max = x_len - 1 - offset;

    int n_min = n_orig_min * K;
    int n_max = n_orig_max * K;

    int len = n_max - n_min + 1;
    if (len <= 0) {
        *out = NULL;
        *out_len = 0;
        *out_offset = 0;
        return NULL;
    }

    int* result = (int*)malloc(len * sizeof(int));
    if (!result) {
        *out = NULL;
        return NULL;
    }

    for (int n = n_min; n <= n_max; n++) {
        int idx_out = n - n_min;
        if (n % K == 0) {
            int n_orig = n / K;
            int idx_orig = offset + n_orig;
            if (idx_orig >= 0 && idx_orig < x_len)
                result[idx_out] = x[idx_orig];
            else
                result[idx_out] = 0;
        } else {
            result[idx_out] = 0;
        }
    }

    *out = result;
    *out_len = len;
    *out_offset = -n_min;
    return result;
}

void print_seq(const char* name, const int* seq, int len, int offset) {
    printf("%s (len=%d, offset=%d):\n", name, len, offset);
    printf("n:    ");
    for (int i = 0; i < len; i++) {
        printf("%4d ", i - offset);
    }
    printf("\nval:  ");
    for (int i = 0; i < len; i++) {
        printf("%4d ", seq[i]);
    }
    printf("\n\n");
}

int main() {
    int x_raw[] = {0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, -1, -2, -1, 0, 1, 2};
    int x_len = sizeof(x_raw) / sizeof(x_raw[0]);
    int zr_offset = 8;

    printf("========== Original Sequence ==========\n");
    print_seq("x", x_raw, x_len, zr_offset);

    printf("========== Downsample Examples (M = 2, 3, 4) ==========\n");
    for (int M = 2; M <= 4; M++) {
        int* gM = NULL;
        int gM_len = 0, gM_offset = 0;
        downsample(x_raw, x_len, zr_offset, M, &gM, &gM_len, &gM_offset);
        printf("Downsample M = %d:\n", M);
        print_seq("gM", gM, gM_len, gM_offset);
        free(gM);
    }

    printf("========== Interpolate Examples (K = 2, 3) ==========\n");
    for (int K = 2; K <= 3; K++) {
        int* fK = NULL;
        int fK_len = 0, fK_offset = 0;
        interpolate(x_raw, x_len, zr_offset, K, &fK, &fK_len, &fK_offset);
        printf("Interpolate K = %d:\n", K);
        print_seq("fK", fK, fK_len, fK_offset);
        free(fK);
    }

    return 0;
}