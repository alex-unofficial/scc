#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>
#include <scc.h>
#include <scc_pthreads.h>

int main(int argc, char **argv) {
	srand(time(NULL));

	char* mtx_fname = NULL;
	if(argc < 2) {
		fprintf(stderr, "Error reading input arguments: %s\nUsage:\t%s mtx_file.mtx\n", 
				strerror(EINVAL), argv[0]);
		exit(EINVAL);
	}
	mtx_fname = argv[1];

	printf("Importing graph %s: ...\r", mtx_fname);
	graph G;
	if(import_graph(mtx_fname, &G)) {
		return -1;
	}
	printf("Importing graph %s: Done\n", mtx_fname);

	printf("\n");

	struct timeval t1, t2;
	double elapsedtime;

	printf("Using serialized SCC algorithm: ...\r");
	size_t *scc_id;
	gettimeofday(&t1, NULL);
	size_t n_scc = scc_coloring(&G, &scc_id);
	gettimeofday(&t2, NULL);
	printf("Using serialized SCC algorithm: Done\n");

	elapsedtime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedtime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	printf("serialized SCC: %f ms\n", elapsedtime);

	printf("\n");

	printf("Using pthreads SCC algorithm: ...\r");
	size_t *p_scc_id;
	gettimeofday(&t1, NULL);
	size_t p_n_scc = p_scc_coloring(&G, &p_scc_id);
	gettimeofday(&t2, NULL);
	printf("Using pthreads SCC algorithm: Done\n");

	elapsedtime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedtime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	printf("pthreads SCC: %f ms\n", elapsedtime);

	printf("\n");

	printf("n_scc = %zu\n", n_scc);
	printf("p_n_scc = %zu\n", p_n_scc);

	for(size_t i = 0 ; i < G.n_verts ; i++) { 
		if(scc_id[i] != p_scc_id[i]) {
			printf(
				"error: non matching value at vertex %zu\nserial scc_id = %zu, pthread scc_id = %zu\n",
				i, scc_id[i], p_scc_id[i]
			);
		}
	}

	free(scc_id);
	free(p_scc_id);

	free_graph(&G);

	return 0;
}
