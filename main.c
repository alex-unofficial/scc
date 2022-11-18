#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>

int main(size_t argc, char **argv) {
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

	size_t *properties = (size_t *) malloc(G.n_verts * sizeof(size_t));
	for(size_t i = 0 ; i < G.n_verts ; i++) properties[i] = 1;
	size_t search_prop = 1;

	size_t root = rand() % G.n_verts;

	size_t *result;
	size_t result_size;

	result_size = forward_bfs(root, &G, search_prop, properties, &result);

	free(properties);

	if(result_size > 0) {
		printf("root = %zu\n", root);
		for(size_t i = 0 ; i < result_size ; i++) {
			printf("%zu ", result[i]);
		}
		printf("\n");

		free(result);
	}


	free_graph(&G);

	return 0;
}
