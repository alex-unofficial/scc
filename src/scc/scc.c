#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>
#include <scc_serial.h>
#include <scc_pthreads.h>
#include <scc_openmp.h>

#define NUMTHREADS 4
#undef NUMTHREADS

int main(int argc, char **argv) {
	char* mtx_fname = NULL;
	if(argc < 2) {
		fprintf(stderr, "Error reading input arguments: %s\nUsage:\t%s mtx_file.mtx\n", 
				strerror(EINVAL), argv[0]);
		exit(EINVAL);
	}
	mtx_fname = argv[1];

	printf("=== importing graph ===\n");
	printf("file: %s\n", mtx_fname);
	graph *G = import_graph(mtx_fname);
	if(G == NULL) return -1;

	printf("\n");

	struct timespec t1, t2;
	double elapsedtime;

	printf("=== serial SCC algorithm ===\n");
	vert_t *scc_id;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	ssize_t n_scc = scc_coloring(G, &scc_id);
	clock_gettime(CLOCK_MONOTONIC, &t2);

	if(n_scc == -1) {
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_nsec - t1.tv_nsec) / 10000000000.0;
	printf("total time: %0.4f sec\n", elapsedtime);

	printf("\n");

	printf("=== pthreads SCC algorithm ===\n");
	vert_t *p_scc_id;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	ssize_t p_n_scc = p_scc_coloring(G, &p_scc_id);
	clock_gettime(CLOCK_MONOTONIC, &t2);

	if(p_n_scc == -1) {
		free(scc_id);
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", p_n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_nsec - t1.tv_nsec) / 1000000000.0;
	printf("total time: %0.4f sec\n", elapsedtime);

	printf("\n");

	printf("=== openmp SCC algorithm ===\n");
	vert_t *omp_scc_id;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	ssize_t omp_n_scc = omp_scc_coloring(G, &omp_scc_id);
	clock_gettime(CLOCK_MONOTONIC, &t2);

	if(omp_n_scc == -1) {
		free(scc_id);
		free(p_scc_id);
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", omp_n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_nsec - t1.tv_nsec) / 1000000000.0;
	printf("total time: %0.4f sec\n", elapsedtime);

	printf("\n");

	printf("=== checking for errors ===\n");
	size_t num_errors = 0;
	if(n_scc != p_n_scc || n_scc != omp_n_scc) {
		printf(
			"number of SCCs:\nserial n_scc = %zd, pthread n_scc = %zd, openmp n_scc = %zd\n",
			n_scc, p_n_scc, omp_n_scc
		);
		num_errors++;
	}
	for(vert_t i = 0 ; i < G->n_verts ; i++) { 
		if(scc_id[i] != p_scc_id[i] || scc_id[i] != omp_scc_id[i]) {
			printf(
				"at vertex %u: serial scc_id = %u, pthread scc_id = %u\n, openmp scc_id = %u\n",
				i, scc_id[i], p_scc_id[i], omp_scc_id[i]
			);
			num_errors++;
		}
	}
	printf("errors found: %zu\n", num_errors);

	free(scc_id);
	free(p_scc_id);
	free(omp_scc_id);

	free_graph(G);

	return 0;
}
