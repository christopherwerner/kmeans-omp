#ifndef PROJECT1_KMEANS_H
#define PROJECT1_KMEANS_H

#include <stdio.h>
#include <stdlib.h>

#define NUM_CLUSTERS 15
#define MAX_ITERATIONS 10000
#define MAX_POINTS 5000

struct point {
    double x, y;
    int cluster;
};

struct kmeans_config {
    char* in_file;
    char* out_file;
    char *test_file;
    int max_points;
    int num_clusters;
    int max_iterations;
};

struct kmeans_config new_config()
{
    struct kmeans_config new_config;
    new_config.out_file = NULL;
    new_config.test_file = NULL;
    new_config.max_points = MAX_POINTS;
    new_config.num_clusters = NUM_CLUSTERS;
    new_config.max_iterations = MAX_ITERATIONS;
    return new_config;
}

struct kmeans_metrics {
    char *label;
    double assignment_seconds;
    double centroids_seconds;
    double total_seconds;
    double max_iteration_seconds;
    int iterations;
    int test;
};

struct kmeans_metrics new_metrics()
{
    struct kmeans_metrics new_metrics;
    new_metrics.label = "no_label_is_assigned";
    new_metrics.centroids_seconds = 0;
    new_metrics.assignment_seconds = 0;
    new_metrics.total_seconds = 0;
    new_metrics.max_iteration_seconds = 0;
    new_metrics.iterations = 0;
    new_metrics.test = 0;
    return new_metrics;
}

void usage()
{
    fprintf(stderr, "Usage: kmeans -f data.csv [-o OUTPUT.CSV] [-i MAX_ITERATIONS] [-n MAX_POINTS] [-k NUM_CLUSTERS]\n");
    exit(1);
}

void print_points(FILE *out, struct point *dataset, int num_points) {
    for (int i = 0; i < num_points; ++i) {
        fprintf(out, "%.2f,%.2f,cluster_%d\n", dataset[i].x, dataset[i].y, dataset[i].cluster);
    }
}

void print_centroids(FILE *out, struct point *centroids, int num_points) {
    for (int i = 0; i < num_points; ++i) {
        fprintf(out, "centroid[%d] is at %.2f,%.2f\n", i, centroids[i].x, centroids[i].y);
    }
}

void print_headers(FILE *out, char **headers, int dimensions) {
    if (headers == NULL) return;

    fprintf(out, "%s", headers[0]);
    for (int i = 1; i < dimensions; ++i) {
        fprintf(out, ",%s", headers[i]);
    }
    // add a 3rd header called "Cluster" to match the Knime output for easier comparison
    fprintf(out, ",Cluster\n");
}

void print_metrics_headers(FILE *out)
{
    fprintf(out, "label,iterations,total,assignments,centroids,max_iteration,test_results");
}

void print_metrics(FILE *out, struct kmeans_metrics metrics)
{
    char *results = "untested";
    switch (metrics.test) {
        case 1:
            results = "passed";
            break;
        case -1:
            results = "FAILED!";
            break;
    }
    fprintf(out, "%20s %d %.3f %.3f %.3fs %.3f %s\n",
            metrics.label, metrics.iterations, metrics.total_seconds,
            metrics.assignment_seconds, metrics.centroids_seconds, metrics.max_iteration_seconds, results);
}

int read_csv(FILE* csv_file, struct point *dataset, int max_points, char *headers[], int *dimensions)
{
    char *line;
    *dimensions = csvheaders(csv_file, headers);
    int max_fields = *dimensions > 2 ? 3 : 2; // max is 2 unless there is a cluster in which case 3
    int count = 0;
    while (count < max_points && (line = csvgetline(csv_file)) != NULL) {
        int num_fields = csvnfield(); // fields on the line
#ifdef DEBUG
        if (num_fields > max_fields) {
            printf("Warning: more that %d fields on line. Ignoring after the first %d: %s", max_fields, max_fields, line);
        }
#endif
        if (num_fields < 2) {
            printf("Warning: found non-empty trailing line. Will stop reading points now: %s", line);
            break;
        }
        else {
            struct point new_point;
            new_point.cluster = -1; // -1 => no cluster yet assigned
            char *x_string = csvfield(0);
            char *y_string = csvfield(1);
            new_point.x = strtod(x_string, NULL);
            new_point.y = strtod(y_string, NULL);

            if (num_fields > 2 && *dimensions > 2) {
                char *cluster_string = csvfield(2);
                // deliberately atoi it producing zero if it does not match a proper cluster
                new_point.cluster = atoi(cluster_string);
            }
            dataset[count] = new_point;
            count++;
        }
    }
    fclose(csv_file);
    return count;
}

int read_csv_file(char* csv_file_name, struct point *dataset, int max_points, char *headers[], int *dimensions)
{
    FILE *csv_file = fopen(csv_file_name, "r");
    if (!csv_file) {
        fprintf(stderr, "Error: cannot read the input file at %s\n", csv_file_name);
        exit(1);
    }
    return read_csv(csv_file, dataset, max_points, headers, dimensions);
}

void write_csv(FILE *csv_file, struct point *dataset, int num_points, char *headers[], int dimensions)
{
    if (headers != NULL) {
        print_headers(csv_file, headers, dimensions);
    }

    print_points(csv_file, dataset, num_points);
}

void write_csv_file(char *csv_file_name, struct point *dataset, int num_points, char *headers[], int dimensions) {
    FILE *csv_file = fopen(csv_file_name, "w");
    if (!csv_file) {
        fprintf(stderr, "Error: cannot write to the output file at %s\n", csv_file_name);
        exit(1);
    }

    write_csv(csv_file, dataset, num_points, headers, dimensions);
}

char* valid_file(char opt, char *filename)
{
    if (access(filename, F_OK ) == -1 ) {
        fprintf(stderr, "Error: The option '%c' expects the name of an existing file (cannot find %s)\n", opt, filename);
        usage();
    }
    return filename;
}

int valid_count(char opt, char *arg)
{
    int value = atoi(arg);
    if (value <= 0) {
        fprintf(stderr, "Error: The option '%c' expects a counting number (got %s)\n", opt, arg);
        usage();
    }
    return value;
}

void validate_config(struct kmeans_config config)
{
    if (!config.in_file) {
        fprintf(stderr, "You must at least provide an input file with -f\n");
        usage();
    }
    printf("Config:");
    printf("Input file    : %-10s\n", config.in_file);
    printf("Output file   : %-10s\n", config.out_file);
    printf("Test file     : %-10s\n", config.test_file);
    printf("Num clusters  : %-10d\n", config.num_clusters);
    printf("Max points    : %-10d\n", config.max_points);
    printf("Max iterations: %-10d\n", config.max_iterations);
}

struct kmeans_config parse_cli(int argc, char *argv[])
{
    int opt;
    struct kmeans_config config = new_config();
    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'

    if (argc < 2) {
        fprintf(stderr, "ERROR: You must at least provide an input file with -f\n");
        usage();
    }

    while((opt = getopt(argc, argv, "f:i:o:k:n:")) != -1)
    {
        switch(opt) {
            case 'f':
                config.in_file = valid_file(optopt, optarg);
                break;
            case 'o':
                config.out_file = optarg;
                break;
            case 't':
                config.test_file = optarg;
                break;
            case 'i':
                config.max_iterations = valid_count(optopt, optarg);
                break;
            case 'n':
                config.max_points = valid_count(optopt, optarg);
                break;
            case 'k':
                config.num_clusters = valid_count(optopt, optarg);
                break;
            case ':':
                printf("Option %c needs a value\n", optopt);
                break;
            case '?':
                printf("unknown option: %c\n", optopt);
                break;
            default:
                printf("Default opts");
        }
    }

    validate_config(config);

    return config;
}
#endif //PROJECT1_KMEANS_H
