#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <graph.h>

int main(size_t argc, char **argv) {
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

	size_t *verts = NULL;
	size_t n_active_verts = get_vertices(&G, &verts);

	for(size_t i = 0; i < n_active_verts ; ++i) {
		size_t v = verts[i];

		size_t n_N;
		size_t *N;
		n_N = get_neighbours(v, &G, &N);

		if(n_N > 0) {
			printf("neighbours %zu:", v);
			for(size_t i = 0 ; i < n_N ; ++i) printf(" %zu", N[i]);
			printf("\n");
			free(N);
		}

		size_t n_P;
		size_t *P;
		n_P = get_predecessors(v, &G, &P);

		if(n_P > 0) {
			printf("predecessors %zu:", v);
			for(size_t i = 0 ; i < n_P ; ++i) printf(" %zu", P[i]);
			printf("\n");
			free(P);
		}

		printf("\n");
	}

	free(verts);

	free_graph(&G);

	return 0;
}
