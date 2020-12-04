#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <omp.h>
#include "kmeans.h"

static char* headers[3];
static int dimensions;

/**
 * Initializes the given array of points to act as initial centroid "representatives" of the
 * clusters, by selecting the first num_clusters points in the dataset.
 *
 * Note that there are many ways to do this for k-means, most of which are better than the
 * approach used here - the most common of which is to use a random sampling of points from
 * the dataset. Since we are performance tuning, however, we want a consistent performance
 * across difference runs of the algorithm, so we simply select the first K points in the dataset
 * where K is the number of clusters.
 *
 * WARNING: The kmeans can fail if there are equal points in the first K in the dataset
 *          such that two or more of the centroids are the same... try to avoid this in
 *          your dataset (TODO fix this later to skip equal centroids)
 *
 * @param dataset array of all points
 * @param centroids uninitialized array of centroids to be filled
 * @param num_clusters the number of clusters (K) for which centroids are created
 */
void initialize_centroids(struct point* dataset, struct point *centroids, int num_clusters)
{
    for (int k = 0; k < num_clusters; ++k) {
        centroids[k] = dataset[k];
    }
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

/**
 * Assigns each point in the dataset to a cluster based on the distance from that cluster
 * @param dataset
 * @param num_points
 * @param centroids
 * @param num_clusters
 * @return
 */
int assign_clusters(struct point* dataset, int num_points, struct point *centroids, int num_clusters)
{
#ifdef DEBUG
    printf("\nStarting assignment phase:\n");
#endif
    int cluster_changes = 0;
    for (int n = 0; n < num_points; ++n) {
        double min_distance = DBL_MAX; // init the min distance to a big number
        int closest_cluster = -1;
        for (int k = 0; k < num_clusters; ++k) {
            // calc the distance passing pointers to points since the distance does not modify them
            double distance_from_centroid = euclidean_distance(&dataset[n], &centroids[k]);
            if (distance_from_centroid < min_distance) {
                min_distance = distance_from_centroid;
                closest_cluster = k;
            }
        }
        // if the point was not already in the closest cluster, move it there and count changes
        if (dataset[n].cluster != closest_cluster) {
            dataset[n].cluster = closest_cluster;
            cluster_changes++;
#ifdef DEBUG
            printf("Assigning (%.0f, %.0f) to cluster %d with centroid (%.2f, %.2f) d = %.2f\n",
                   dataset[n].x, dataset[n].y, closest_cluster,
                   centroids[closest_cluster].x, centroids[closest_cluster].y, min_distance);
#endif
        }
    }
    return cluster_changes;
}

/**
 * Calculates new centroids for the clusters of the given dataset by finding the
 * mean x and y coordinates of the current members of the cluster for each cluster.
 *
 * The centroids are set in the array passed in, which is expected to be pre-allocated
 * and contain the previous centroids: these are overwritten by the new values.
 *
 * @param dataset set of all points with current cluster assigments
 * @param num_points number of points in the dataset
 * @param centroids array to hold the centroids - already allocated
 * @param num_clusters number of clusters - hence size of the centroids array
 */
void calculate_centroids(struct point* dataset, int num_points, struct point *centroids, int num_clusters)
{
    double sum_of_x_per_cluster[num_clusters];
    double sum_of_y_per_cluster[num_clusters];
    int num_points_in_cluster[num_clusters];
    for (int k = 0; k < num_clusters; ++k) {
        sum_of_x_per_cluster[k] = 0.0;
        sum_of_y_per_cluster[k] = 0.0;
        num_points_in_cluster[k] = 0;
    }

    // loop over all points in the database and sum up
    // the x coords of clusters to which each belongs
    for (int n = 0; n < num_points; ++n) {
        // use pointer to struct to avoid creating unnecessary copy in memory
        struct point *p = &dataset[n];
        int k = p->cluster;
        sum_of_x_per_cluster[k] += p->x;
        sum_of_y_per_cluster[k] += p->y;
        // count the points in the cluster to get a mean later
        num_points_in_cluster[k]++;
    }

    // the new centroids are at the mean x and y coords of the clusters
    for (int k = 0; k < num_clusters; ++k) {
        struct point new_centroid;
        // mean x, mean y => new centroid
        new_centroid.x = sum_of_x_per_cluster[k] / num_points_in_cluster[k];
        new_centroid.y = sum_of_y_per_cluster[k] / num_points_in_cluster[k];
        centroids[k] = new_centroid;
    }
}

int main(int argc, char* argv [])
{
    struct kmeans_config config = parse_cli(argc, argv);

    struct point *dataset = malloc(config.max_points * sizeof(struct point));
    char* csv_file_name = valid_file('f', config.in_file);
    int num_points = read_csv_file(csv_file_name, dataset, config.max_points, headers, &dimensions);

    // K-Means Algo Step 1: initialize the centroids
    struct point *centroids = malloc(config.num_clusters * sizeof(struct point));
    initialize_centroids(dataset, centroids, config.num_clusters);

    // we deliberately skip the centroid initialization phase in calculating the
    // total time as it is constant and never optimized
    double start_time = omp_get_wtime();
#ifdef DEBUG
    printf("\nDatabase:\n");
    print_headers(stdout, headers, dimensions);
    print_points(stdout, dataset, num_points);
    printf("\nCentroids:\n");
    print_centroids(stdout, centroids, config.num_clusters);
#endif
    int cluster_changes = num_points;
    int iterations = 0;

    // set up a metrics struct to hold timing and other info for comparison
    struct kmeans_metrics metrics = new_metrics();
    metrics.label = config.label;
    metrics.max_iterations = config.max_iterations;
    metrics.num_clusters = config.num_clusters;
    metrics.num_points = num_points;

    while (cluster_changes > 0 && iterations < config.max_iterations) {
        // K-Means Algo Step 2: assign every point to a cluster (closest centroid)
        double start_iteration = omp_get_wtime();
        double start_assignment = start_iteration;
        cluster_changes = assign_clusters(dataset, num_points, centroids, config.num_clusters);
        double assignment_seconds = omp_get_wtime() - start_assignment;

        metrics.assignment_seconds += assignment_seconds;

#ifdef DEBUG
        printf("\n%d clusters changed after assignment phase. New assignments:\n", cluster_changes);
        print_points(stdout, dataset, num_points);
        printf("Time taken: %.3f seconds total in assignment so far: %.3f seconds",
               assignment_seconds, metrics.assignment_seconds);
#endif
        // K-Means Algo Step 3: calculate new centroids: one at the center of each cluster
        double start_centroids = omp_get_wtime();
        calculate_centroids(dataset, num_points, centroids, config.num_clusters);
        double centroids_seconds = omp_get_wtime() - start_centroids;
        metrics.centroids_seconds += centroids_seconds;

#ifdef DEBUG
        printf("New centroids calculated New assignments:\n");
        print_centroids(stdout, centroids, config.num_clusters);
        printf("Time taken: %fseconds total in centroid calculation so far: %fseconds",
               centroids_seconds, metrics.centroids_seconds);
#endif
//#ifdef CALC_MAX_ITERATION_TIME
        // potentially costly calculation may skew stats, hence only in ifdef
        double iteration_seconds = omp_get_wtime() - start_iteration;
        if (iteration_seconds > metrics.max_iteration_seconds) {
            metrics.max_iteration_seconds = iteration_seconds;
        }
//#endif
        iterations++;
    }
    metrics.total_seconds = omp_get_wtime() - start_time;
    metrics.used_iterations = iterations;

    printf("\nEnded after %d iterations with %d changed clusters\n", iterations, cluster_changes);
    printf("Writing output to %s\n", config.out_file);
    write_csv_file(config.out_file, dataset, num_points, headers, dimensions);
#ifdef DEBUG
    write_csv(stdout, dataset, num_points, headers, dimensions);
#endif

    if (config.test_file) {
        char* test_file_name = valid_file('t', config.test_file);
        printf("Comparing results against test file: %s\n", config.test_file);
        metrics.test_result = test_results(test_file_name, dataset, num_points);
    }

    if (config.metrics_file) {
        // metrics file may or may not already exist
        printf("Reporting metrics to: %s\n", config.metrics_file);
        write_metrics_file(config.metrics_file, &metrics);
    }

    print_metrics_headers(stdout);
    print_metrics(stdout, &metrics);
    return 0;
}

