#include "scc.h"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <string.h>

int is_trivial_scc(size_t v, graph *G) {
	size_t *N;
	size_t n_N = get_neighbours(v, G, &N);

	size_t *P;
	size_t n_P = get_predecessors(v, G, &P);

	if(n_N == 0) {
		if(n_P > 0) free(P);
		return 1;
	}

	if(n_P == 0) {
		if(n_N > 0) free(N);
		return 1;
	}

	if((n_N == 1 && N[0] == v) || (n_P == 1 && P[0] == v)) {
		free(N);
		free(P);
		return 1;
	}

	free(N);
	free(P);
	return 0;
}


size_t scc_coloring(graph *G, size_t **scc_id) {
	*scc_id = (size_t *) malloc(G->n_verts * sizeof(size_t));
	if(*scc_id == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
		return 0;
	}

	size_t n_scc = 0;
	
	// remove trivial sccs
	int removed_vertex = 1;
	while(removed_vertex) {
		removed_vertex = 0;

		for(size_t v = 0 ; v < G->n_verts ; ++v) {
			if(is_vertex(v, G) && is_trivial_scc(v, G)) {
				(*scc_id)[v] = v;
				n_scc += 1;

				remove_vertex(v, G);
				removed_vertex = 1;
			}
		}
	}

	// main loop
	while(G->n_active_verts > 0) {
		size_t *colors = (size_t *) malloc(G->n_verts * sizeof(size_t));
		if(colors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
			free(*scc_id);
			return 0;
		}
		for(size_t v = 0 ; v < G->n_verts ; ++v) colors[v] = v;

		int changed_color = 1;
		while(changed_color) {
			changed_color = 0;

			for(size_t v = 0 ; v < G->n_verts ; ++v) {
				if(is_vertex(v, G)) {
					size_t *predecessors;
					size_t n_predecessors = get_predecessors(v, G, &predecessors);

					if(n_predecessors > 0) {
						for(size_t i = 0 ; i < n_predecessors ; ++i) {
							size_t u = predecessors[i];
							if(colors[v] > colors[u]) {
								colors[v] = colors[u];
								changed_color = 1;
							}
						}

						free(predecessors);
					}

				}
			}
		}

		size_t *unique_colors = (size_t *) malloc(G->n_verts * sizeof(size_t));
		if(unique_colors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
			free(*scc_id);
			free(colors);
			return 0;
		}
		size_t n_colors = 0;

		for(size_t v = 0 ; v < G->n_verts ; ++v) {
			if(is_vertex(v, G) && colors[v] == v) 
				unique_colors[n_colors++] = v;
		}

		unique_colors = (size_t *) realloc(unique_colors, n_colors * sizeof(size_t));

		for(size_t i = 0 ; i < n_colors ; ++i) {
			size_t c = unique_colors[i];

			size_t *scc_c;
			size_t n_scc_c = backward_bfs(c, G, c, colors, &scc_c);

			if(n_scc_c > 0) {
				for(size_t j = 0 ; j < n_scc_c ; ++j) {
					size_t v = scc_c[j];
					(*scc_id)[v] = c;
				}
				n_scc += 1;

				remove_vertices(scc_c, n_scc_c, G);

				free(scc_c);
			}
		}

		free(unique_colors);
		free(colors);
	}

	return n_scc;
}
