#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void state_matrices(double A[2][2], double B[2], double C[2], double *D);

int main(int argc, char *argv[])
{
    double a1 = 0.180000;
    double a2 = -0.130000;
    double b0 = 1.0;

    if (argc == 4) {
        a1 = atof(argv[1]);
        a2 = atof(argv[2]);
        b0 = atof(argv[3]);
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [a1 a2 b0]\n", argv[0]);
        return 1;
    }

    double A[2][2];
    double B[2];
    double C[2];
    double D;
    state_matrices(A, B, C, &D);

    double tol = 1e-9;
    int fail = 0;

    if (fabs(A[0][0] - a1) > tol) fail = 1;
    if (fabs(A[0][1] - a2) > tol) fail = 1;
    if (fabs(A[1][0] - 1.0) > tol) fail = 1;
    if (fabs(A[1][1] - 0.0) > tol) fail = 1;
    if (fabs(B[0] - b0) > tol) fail = 1;
    if (fabs(B[1] - 0.0) > tol) fail = 1;
    if (fabs(C[0] - 1.0) > tol) fail = 1;
    if (fabs(C[1] - 0.0) > tol) fail = 1;
    if (fabs(D - 0.0) > tol) fail = 1;

    if (fail) {
        printf("Matrix check failed.\n");
        return 1;
    }

    double x[10] = {1.0, 0.5, -0.3, 0.0, 0.7, 1.2, -0.4, 0.3, 0.0, -0.5};
    double y_diff[10] = {0};
    double s[2] = {0.0, 0.0};
    double y_state[10] = {0};

    for (int n = 0; n < 10; n++) {
        double y = a1 * (n >= 1 ? y_diff[n-1] : 0.0) + a2 * (n >= 2 ? y_diff[n-2] : 0.0) + b0 * x[n];
        y_diff[n] = y;

        double sn1[2];
        sn1[0] = A[0][0] * s[0] + A[0][1] * s[1] + B[0] * x[n];
        sn1[1] = A[1][0] * s[0] + A[1][1] * s[1] + B[1] * x[n];
        y_state[n] = C[0] * sn1[0] + C[1] * sn1[1] + D * x[n];
        s[0] = sn1[0];
        s[1] = sn1[1];
    }

    for (int n = 0; n < 10; n++) {
        if (fabs(y_diff[n] - y_state[n]) > 1e-9) {
            printf("Output mismatch at n=%d: diff=%.12f state=%.12f\n", n, y_diff[n], y_state[n]);
            return 1;
        }
    }

    printf("PASS\n");
    return 0;
}
