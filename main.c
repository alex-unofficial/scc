#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>
#include <scc.h>

int main(int argc, char **argv) {
	srand(time(NULL));

	char* mtx_fname = NULL;
	if(argc < 2) {
		fprintf(stderr, "Error reading input arguments: %s\nUsage:\t%s mtx_file.mtx\n", 
				strerror(EINVAL), argv[0]);
		exit(EINVAL);
	}
	mtx_fname = argv[1];

	graph G;
	if(import_graph(mtx_fname, &G)) {
		return -1;
	}

	size_t *scc_id;
	size_t n_scc = scc_coloring(&G, &scc_id);

	printf("n_scc = %zu\n\n", n_scc);
	printf("vertex\tscc_id\n");
	for(size_t i = 0 ; i < G.n_verts ; i++) {
		printf(" %zu\t %zu\n", i, scc_id[i]);
	}

	free(scc_id);

	free_graph(&G);

	return 0;
}
