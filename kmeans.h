#ifndef KMEANS_H
#define KMEANS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <omp.h>
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
    bool silent;
    bool quiet;
};

extern struct kmeans_config new_config();

struct kmeans_metrics {
    char *label; // label for metrics row from -l command line arg
    double assignment_seconds;    // total time spent assigning points to clusters in every iteration
    double centroids_seconds;     // total time spent calculating new centroids in every iteration
    double total_seconds;         // total time in seconds for the run
    double max_iteration_seconds; // time taken by the slowest iteration of the whole algo
    int used_iterations; // number of actual iterations needed to complete clustering
    int test_result;     // 0 = not tested, 1 = passed, -1 = failed comparison with expected data
    int num_points;      // number or points in the file limited to max from -n command line arg
    int num_clusters;    // number of clusters from  -k command line arg
    int max_iterations;  // max iterations from -i command line arg
    int omp_max_threads; // OMP max threads, usually set by OMP_NUM_THREADS env var or an function call
    // Next 2 are OMP schedule kind (static, dynamic, auto) and chunk size, set by OMP_SCHEDULE var.
    // See: https://gcc.gnu.org/onlinedocs/libgomp/omp_005fget_005fschedule.html#omp_005fget_005fschedule
    int omp_schedule_kind;
    int omp_chunk_size;
};

extern struct kmeans_metrics new_metrics();
extern const char *p_to_s(struct point *p);
extern double euclidean_distance(struct point *p1, struct point *p2);
extern void usage();
extern void print_points(FILE *out, struct point *dataset, int num_points);
extern void print_headers(FILE *out, char **headers, int dimensions);
extern void print_metrics_headers(FILE *out);
extern void print_centroids(FILE *out, struct point *centroids, int num_points);

extern void print_metrics(FILE *out, struct kmeans_metrics *metrics);
int read_csv_file(char* csv_file_name, struct point *dataset, int max_points, char *headers[], int *dimensions);
extern int read_csv(FILE* csv_file, struct point *dataset, int max_points, char *headers[], int *dimensions);
extern void write_csv_file(char *csv_file_name, struct point *dataset, int num_points, char *headers[], int dimensions);
extern void write_csv(FILE *csv_file, struct point *dataset, int num_points, char *headers[], int dimensions);

extern void write_metrics_file(char *metrics_file_name, struct kmeans_metrics *metrics) ;

extern char* valid_file(char opt, char *filename);
extern int valid_count(char opt, char *arg);
extern void validate_config(struct kmeans_config config);

extern int test_results(struct kmeans_config *config, char* test_file_name, struct point *dataset, int num_points);

extern struct kmeans_config parse_cli(int argc, char *argv[]);

extern void debug_assignment(struct point *p, int closest_cluster, struct point *centroid, double min_distance);

// help with debugging OMP
extern int omp_schedule_kind(int *chunk_size);
extern void omp_debug(char *msg);

#endif