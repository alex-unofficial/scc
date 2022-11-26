#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>
#include <scc.h>
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

	struct timeval t1, t2;
	double elapsedtime;

	printf("=== serial SCC algorithm ===\n");
	vert_t *scc_id;
	gettimeofday(&t1, NULL);
	ssize_t n_scc = scc_coloring(G, &scc_id);
	gettimeofday(&t2, NULL);

	if(n_scc == -1) {
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
	printf("total time: %0.4f sec\n", elapsedtime);

	printf("\n");

	printf("=== pthreads SCC algorithm ===\n");
	vert_t *p_scc_id;
	gettimeofday(&t1, NULL);
	ssize_t p_n_scc = p_scc_coloring(G, &p_scc_id);
	gettimeofday(&t2, NULL);

	if(p_n_scc == -1) {
		free(scc_id);
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", p_n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
	printf("total time: %0.4f sec\n", elapsedtime);

	printf("\n");

	printf("=== openmp SCC algorithm ===\n");
	vert_t *omp_scc_id;
	gettimeofday(&t1, NULL);
	ssize_t omp_n_scc = omp_scc_coloring(G, &omp_scc_id);
	gettimeofday(&t2, NULL);

	if(omp_n_scc == -1) {
		free(scc_id);
		free(p_scc_id);
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", omp_n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
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
