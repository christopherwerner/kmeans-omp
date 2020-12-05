#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include "csvhelper.h"
#include "kmeans.h"
#include <math.h>

struct kmeans_config new_config()
{
    struct kmeans_config new_config;
    new_config.out_file = NULL;
    new_config.test_file = NULL;
    new_config.metrics_file = NULL;
    new_config.label = "no-label";
    new_config.max_points = MAX_POINTS;
    new_config.num_clusters = NUM_CLUSTERS;
    new_config.max_iterations = MAX_ITERATIONS;
    return new_config;
}

struct kmeans_metrics new_metrics()
{
    struct kmeans_metrics new_metrics;
    new_metrics.label = "no_label_is_assigned";
    new_metrics.centroids_seconds = 0;
    new_metrics.assignment_seconds = 0;
    new_metrics.total_seconds = 0;
    new_metrics.max_iteration_seconds = 0;
    new_metrics.used_iterations = 0;
    new_metrics.test_result = 0; // zero = no test performed
    new_metrics.num_points = 0;
    new_metrics.num_clusters = 0;
    new_metrics.max_iterations = 0;
    return new_metrics;
}

/**
 * Calculate the euclidean distance between two points.
 *
 * That is, the square root of the sum of the squares of the distances between coordinates
 *
 * Note that most k-means algorithms work with the pure sum of squares - so the _square_ of
 * the distance, since we really only need the _relative_ distances to assign a point to
 * the right cluster - and square-root is a slow function.
 * But since this is an exercise in performance tuning, we'll do it the slow way with square roots
 * so we can better see how much performance improves when we add OMP
 *
 * The points are expected as pointers since the method does not change them and memory and time is
 * saved by not copying the structs unnecessarily.
 *
 * @param p1 pointer to first point in 2 dimensions
 * @param p2 ponter to second point in 2 dimensions
 * @return geometric distance between the 2 points
 */
double euclidean_distance(struct point *p1, struct point *p2)
{
    double square_diff_x = (p2->x - p1->x) * (p2->x - p1->x);
    double square_diff_y = (p2->y - p1->y) * (p2->y - p1->y);
    double square_dist = square_diff_x + square_diff_y;
    // most k-means algorithms would stop here and return the square of the euclidean distance
    // because its faster, and we only need comparative values for clustering, but since this
    // is an exercise in performance tuning, we'll do it the slow way with square roots
    double dist = sqrt(square_dist);
#ifdef DEBUG

    //   printf("Distance from (%f, %f) -> (%f, %f) = %f\n", p1.x, p1.y, p2.x, p2.y, dist);
//    printf("x^2 = %f, y^2 = %f, d^2 = %f\n", square_diff_x, square_diff_y, square_dist);
#endif
    return dist;
}

void usage()
{
    fprintf(stderr, "Usage: kmeans -f data.csv [-o OUTPUT.CSV] [-i MAX_ITERATIONS] [-n MAX_POINTS] [-k NUM_CLUSTERS] [-t TESTFILE.CSV]\n");
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
    fprintf(out, "label,used_iterations,total_seconds,assignments_seconds,"
                 "centroids_seconds,max_iteration_seconds,num_points,"
                 "num_clusters,max_iterations,test_results\n");
}

/**
 * Print the results of the run with timing numbers in a single row to go in a csv file
 * @param out output file pointer
 * @param metrics metrics object
 */
void print_metrics(FILE *out, struct kmeans_metrics *metrics)
{
    char *test_results = "untested";
    switch (metrics->test_result) {
        case 1:
            test_results = "passed";
            break;
        case -1:
            test_results = "FAILED!";
            break;
    }
    fprintf(out, "%s,%d,%f,%f,%f,%f,%d,%d,%d,%s\n",
            metrics->label, metrics->used_iterations, metrics->total_seconds,
            metrics->assignment_seconds, metrics->centroids_seconds, metrics->max_iteration_seconds,
            metrics->num_points, metrics->num_clusters, metrics->max_iterations,
            test_results);
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
                int cluster;
                char prefix[200];
                int result = sscanf(cluster_string,"%[^0-9]%d", prefix, &cluster);
                new_point.cluster = cluster;
            }
            dataset[count] = new_point;
            count++;
        }
    }
    fclose(csv_file);
    return count;
}

/**
 *
 * Write a Comma-Separated-Values file with points and cluster assignments
 * to the specified file path.
 *
 * IF the file exists it is silently overwritten.
 *
 * @param csv_file_name absolute path to the file to be written
 * @param dataset array of all points with cluster information
 * @param num_points size of the array of points
 * @param headers optional headers to put at the top of the file
 * @param dimensions number of headers
*/
int read_csv_file(char* csv_file_name, struct point *dataset, int max_points, char *headers[], int *dimensions)
{
    FILE *csv_file = fopen(csv_file_name, "r");
    if (!csv_file) {
        fprintf(stderr, "Error: cannot read the input file at %s\n", csv_file_name);
        exit(1);
    }
    return read_csv(csv_file, dataset, max_points, headers, dimensions);
}

/**
 * Write a Comma-Separated-Values file with points and cluster assignments to a file pointer.
 *
 * @param csv_file pointer to the file to write to
 * @param dataset array of all points with cluster information
 * @param num_points size of the array of points
 * @param headers optional headers to put at the top of the file
 * @param dimensions number of headers
 */
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

void write_metrics_file(char *metrics_file_name, struct kmeans_metrics *metrics) {
    char *mode = "a"; // default to append to the metrics file
    bool first_time = false;
    if (access(metrics_file_name, F_OK ) == -1 ) {
        // first time - lets change the mode to "w" and append
        fprintf(stdout, "Creating metrics file and adding headers: %s", metrics_file_name);
        first_time = true;
        mode = "w";
    }
    FILE *metrics_file = fopen(metrics_file_name, mode);
    if (first_time) {
        print_metrics_headers(metrics_file);
    }

    print_metrics(metrics_file, metrics);
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
    printf("Metrics file  : %-10s\n", config.metrics_file);
    printf("Num clusters  : %-10d\n", config.num_clusters);
    printf("Max points    : %-10d\n", config.max_points);
    printf("Max iterations: %-10d\n", config.max_iterations);
}

/**
 * Compares the dataset against test file.
 *
 * If every point in the dataset has a matching point at the same position in the
 * test dataset from the test file, and the clusters match, then 1 is returned,
 * otherwise -1 is returned indicating a failure.
 *
 * Note that the test file may have more points than the dataset - trailing points are ignored
 * in this case - but if it has fewer points, this is considered a test failure.
 *
 * The method returns -1 after the first failure.
 *
 * @param config
 * @param dataset
 * @param num_points
 * @return 1 or -1 if the files match
 */
int test_results(char* test_file_name, struct point *dataset, int num_points)
{
    int result = 1;
    struct point *testset = malloc(MAX_POINTS * sizeof(struct point));
    int test_dimensions;
    static char* test_headers[3];
    int num_test_points = read_csv_file(test_file_name, testset, num_points, test_headers, &test_dimensions);
    if (num_test_points < num_points) {
        fprintf(stderr, "Test failed. The test dataset has only %d records, but needs at least %d",
                num_test_points, num_points);
        result = 1;
    }
    else {
        for (int n = 0; n < num_points; ++n) {
            struct point *p = &dataset[n];
            struct point *test_p = &testset[n];
            if (test_p->x == p->x && test_p->y == p->y) {
                if (test_p->cluster != p->cluster) {
                    // points match but assigned to different clusters
                    fprintf(stderr, "Test failure at %d: (%.2f,%.2f) result cluster: %d does not match test: %d\n",
                            n+1, p->x, p->y, p->cluster, test_p->cluster);
                    result = -1;
                    break; // give up comparing
                }
#ifdef DEBUG
                else {
                    fprintf(stdout, "Test success at %d: (%.2f,%.2f) clusters match: %d\n",
                            n+1, p->x, p->y, p->cluster);

                }
#endif
            }
            else {
                // points themselves are different
                fprintf(stderr, "Test failure at %d: %.2f,%.2f does not match test point: %.2f,%.2f\n",
                        n+1, p->x, p->y, test_p->x, test_p->y);
                result = -1;
                break; // give up comparing
            }
        }
    }
    return result;
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

    while((opt = getopt(argc, argv, "f:i:o:k:n:l:t:m:")) != -1)
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
            case 'm':
                config.metrics_file = optarg;
                break;
            case 'l':
                config.label = optarg;
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
                fprintf(stderr, "ERROR: Option %c needs a value\n", optopt);
                usage();
                break;
            case '?':
                fprintf(stderr, "ERROR: Unknown option: %c\n", optopt);
                usage();
            default:
                printf("Default opts");
        }
    }

    validate_config(config);

    return config;
}
