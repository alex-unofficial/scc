#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>
#include <scc_serial.h>

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
	free(scc_id);

	free_graph(G);

	return 0;
}
