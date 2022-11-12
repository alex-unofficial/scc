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
	import_graph(mtx_fname, &G);

	printf("CSR:\n");
	for(size_t i = 0 ; i < G.n_edges ; ++i) printf("%d ", G.csr_col_id[i]);
	printf("\n");
	for(size_t i = 0 ; i <= G.n_verts ; ++i) printf("%d ", G.csr_row_id[i]);
	printf("\n");
	printf("\n");

	printf("CSC:\n");
	for(size_t i = 0 ; i < G.n_edges ; ++i) printf("%d ", G.csc_row_id[i]);
	printf("\n");
	for(size_t i = 0 ; i <= G.n_verts ; ++i) printf("%d ", G.csc_col_id[i]);
	printf("\n");

	free_graph(&G);

	return 0;
}
