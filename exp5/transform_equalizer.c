#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static int parse_named_double(const char *line, const char *key, double *value)
{
    if (line == NULL || key == NULL || value == NULL) {
        return 0;
    }

    char buffer[256];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(buffer, " \t\r\n", &saveptr);
    while (token != NULL) {
        if (strcmp(token, key) == 0) {
            token = strtok_r(NULL, " \t\r\n", &saveptr);
            while (token != NULL) {
                char *endptr = NULL;
                double value_candidate = strtod(token, &endptr);
                if (endptr != token && *endptr == '\0') {
                    *value = value_candidate;
                    return 1;
                }
                token = strtok_r(NULL, " \t\r\n", &saveptr);
            }
            return 0;
        }
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    return 0;
}

static int parse_number_line(const char *line, double *value)
{
    if (line == NULL || value == NULL) {
        return 0;
    }

    const char *ptr = line;
    while (isspace((unsigned char)*ptr)) {
        ptr++;
    }
    if (*ptr == '\0' || *ptr == '#' || *ptr == ';') {
        return 0;
    }

    char *endptr = NULL;
    double result = strtod(ptr, &endptr);
    if (endptr == ptr) {
        return 0;
    }
    *value = result;
    return 1;
}

static int apply_H_filter(const double *input, double *output, int length, double ai, double bi)
{
    double y_prev1 = 0.0;
    double y_prev2 = 0.0;
    for (int i = 0; i < length; ++i) {
        double y = input[i] + ai * y_prev1 - bi * y_prev2;
        if (!isfinite(y)) {
            y = 0.0;
        }
        output[i] = y;
        y_prev2 = y_prev1;
        y_prev1 = y;
    }
    return 1;
}

static int apply_G_filter(const double *input, double *output, int length, double ai, double bi)
{
    double x_prev1 = 0.0;
    double x_prev2 = 0.0;
    for (int i = 0; i < length; ++i) {
        double y = input[i] - ai * x_prev1 + bi * x_prev2;
        if (!isfinite(y)) {
            y = 0.0;
        }
        output[i] = y;
        x_prev2 = x_prev1;
        x_prev1 = input[i];
    }
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    FILE *fin = fopen(input_path, "r");
    if (fin == NULL) {
        return 1;
    }

    double sample_rate = 0.0;
    int length = 0;
    const double a_i = -0.041;
    const double b_i = 0.514;

    char line[256];
    int sample_count = 0;
    double *samples = NULL;
    int got_sample_rate = 0;
    int got_length = 0;

    while (fgets(line, sizeof(line), fin) != NULL) {
        if (!got_sample_rate && parse_named_double(line, "sample_rate", &sample_rate)) {
            got_sample_rate = 1;
            continue;
        }
        if (!got_length) {
            double value = 0.0;
            if (parse_named_double(line, "length", &value)) {
                length = (int)value;
                if (length < 0) {
                    length = 0;
                }
                got_length = 1;
                continue;
            }
        }

        if (got_sample_rate && got_length && sample_count < length) {
            double sample = 0.0;
            if (parse_number_line(line, &sample)) {
                if (!isfinite(sample)) {
                    sample = 0.0;
                }
                if (samples == NULL) {
                    samples = (double *)calloc((size_t)length, sizeof(double));
                    if (samples == NULL) {
                        fclose(fin);
                        return 1;
                    }
                }
                samples[sample_count++] = sample;
            }
        }
    }

    fclose(fin);

    if (!got_sample_rate || !got_length || length <= 0) {
        free(samples);
        return 1;
    }

    if (samples == NULL) {
        samples = (double *)calloc((size_t)length, sizeof(double));
        if (samples == NULL) {
            return 1;
        }
    }

    double *after_H = (double *)calloc((size_t)length, sizeof(double));
    if (after_H == NULL) {
        free(samples);
        return 1;
    }

    double *after_G = (double *)calloc((size_t)length, sizeof(double));
    if (after_G == NULL) {
        free(samples);
        free(after_H);
        return 1;
    }

    if (!apply_H_filter(samples, after_H, length, a_i, b_i)) {
        free(samples);
        free(after_H);
        free(after_G);
        return 1;
    }

    if (!apply_G_filter(after_H, after_G, length, a_i, b_i)) {
        free(samples);
        free(after_H);
        free(after_G);
        return 1;
    }

    FILE *fout = fopen(output_path, "w");
    if (fout == NULL) {
        free(samples);
        free(after_H);
        free(after_G);
        return 1;
    }

    for (int i = 0; i < length; ++i) {
        fprintf(fout, "%.12f\n", after_G[i]);
    }

    fclose(fout);
    free(samples);
    free(after_H);
    free(after_G);
    (void)sample_rate;

    return 0;
}
