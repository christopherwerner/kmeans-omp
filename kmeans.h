#ifndef KMEANS_H
#define KMEANS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include "csvhelper.h"

#define NUM_CLUSTERS 15
#define MAX_ITERATIONS 10000
#define MAX_POINTS 5000

struct point {
    double x, y;
    int cluster;
};

struct kmeans_config {
    char *in_file;
    char *out_file;
    char *test_file;
    char *metrics_file;
    char *label;
    int max_points;
    int num_clusters;
    int max_iterations;
};

extern struct kmeans_config new_config();

struct kmeans_metrics {
    char *label;
    double assignment_seconds;
    double centroids_seconds;
    double total_seconds;
    double max_iteration_seconds;
    int used_iterations;
    int test_result;
    int num_points;
    int num_clusters;
    int max_iterations;
};

extern struct kmeans_metrics new_metrics();

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
extern double euclidean_distance(struct point *p1, struct point *p2);

extern void usage();

extern void print_points(FILE *out, struct point *dataset, int num_points);

extern void print_headers(FILE *out, char **headers, int dimensions);

extern void print_metrics_headers(FILE *out);

/**
 * Print the results of the run with timing numbers in a single row to go in a csv file
 * @param out output file pointer
 * @param metrics metrics object
 */
extern void print_metrics(FILE *out, struct kmeans_metrics *metrics);

extern int read_csv(FILE* csv_file, struct point *dataset, int max_points, char *headers[], int *dimensions);

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
int read_csv_file(char* csv_file_name, struct point *dataset, int max_points, char *headers[], int *dimensions);

/**
 * Write a Comma-Separated-Values file with points and cluster assignments to a file pointer.
 *
 * @param csv_file pointer to the file to write to
 * @param dataset array of all points with cluster information
 * @param num_points size of the array of points
 * @param headers optional headers to put at the top of the file
 * @param dimensions number of headers
 */
extern void write_csv_file(char *csv_file_name, struct point *dataset, int num_points, char *headers[], int dimensions);

extern void write_metrics_file(char *metrics_file_name, struct kmeans_metrics *metrics) ;

extern char* valid_file(char opt, char *filename);

extern int valid_count(char opt, char *arg);

extern void validate_config(struct kmeans_config config);

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
extern int test_results(char* test_file_name, struct point *dataset, int num_points);


extern struct kmeans_config parse_cli(int argc, char *argv[]);


#endif