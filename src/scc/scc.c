#include "scc.h"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <string.h>

/* Returns true if v is a trivial SCC
 *
 * this is the case if v has no neighbours or no predecessors
 * or if its only neighbour/predecessor is itself
 */
int is_trivial_scc(size_t v, const graph *G, const size_t *is_vertex) {
	size_t *N;
	size_t n_N = get_neighbours(v, G, is_vertex, &N);

	size_t *P;
	size_t n_P = get_predecessors(v, G, is_vertex, &P);

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


/* Implements the graph coloring algorithm to find the SCCs of G
 *
 * takes as input the graph G and a double pointer where the result will 
 * be stored. returns the number of sccs.
 *
 * the graph G is assumed to be complete (all vertices active) and by the end
 * of the algorithm the graph G will be empty (all vetices inactive)
 *
 * scc_id is of size n_verts
 * if v belongs to the scc with id c then: scc_id[v] = c
 */
size_t scc_coloring(const graph *G, size_t **scc_id) {
	
	size_t *is_vertex = (size_t *) malloc(G->n_verts * sizeof(size_t));
	if(is_vertex == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
		return 0;
	}
	for(size_t v = 0 ; v < G->n_verts ; ++v) is_vertex[v] = 1;
	size_t n_active_verts = G->n_verts;

	// allocate the memory required for the scc_id array
	*scc_id = (size_t *) malloc(G->n_verts * sizeof(size_t));
	if(*scc_id == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));

		free(is_vertex);
		return 0;
	}

	// initialize n_sccs to 0
	size_t n_scc = 0;
	
	// remove trivial sccs 
	// the loop will run as long as there are new vertices removed 
	// since removing a vertex may lead to another vertex becoming trivial
	int removed_vertex = 1;
	while(removed_vertex) {
		removed_vertex = 0;

		// loop over all vertices
		for(size_t v = 0 ; v < G->n_verts ; ++v) {
			// check if the vertex is active, and then if it is trivial
			if(is_vertex[v] && is_trivial_scc(v, G, is_vertex)) {
				// if it is, set scc_id for the vertex to be itself
				// and increase the number of sccs
				(*scc_id)[v] = v;
				n_scc += 1;

				// finally remove the vertex from the graph
				is_vertex[v] = 0;
				n_active_verts -= 1;

				removed_vertex = 1;
			}
		}
	}

	// the core loop of the algorithm
	// this will run as long as G is non empty
	while(n_active_verts > 0) {
		// initialize the colors array as colors(v) = v for each v in G
		size_t *colors = (size_t *) malloc(G->n_verts * sizeof(size_t));
		if(colors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
			free(is_vertex);
			free(*scc_id);
			return 0;
		}
		for(size_t v = 0 ; v < G->n_verts ; ++v) colors[v] = v;

		// this loop will run as long as at least one vertex changed colors in
		// the last iteration since a vertex changing color might end up changing
		// the color of its neighbours in the next iteration.
		int changed_color = 1;
		while(changed_color) {
			changed_color = 0;

			// we loop over all the vertives v in the graph (checking if the v is active)
			for(size_t v = 0 ; v < G->n_verts ; ++v) {
				if(is_vertex[v]) {
					// we get the predecessors of the vertex v (vertices u such that [u, v] in G)
					// because we want to write in one memory position (colors[v])
					// as opposed to every u for each v. this is useful for 
					// the parallelization since memory locations the treads
					// write to will not interfere.

					size_t *predecessors;
					size_t n_predecessors = get_predecessors(v, G, is_vertex, &predecessors);

					if(n_predecessors > 0) {
						// then we set colors[v] to be the minimum of its predecessors (or itself)
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


		// after the coloring is finished we need to find all the unique colors c in the colors array
		// there may be up to n_verts unique colors (one for each vertex)
		size_t *unique_colors = (size_t *) malloc(G->n_verts * sizeof(size_t));
		if(unique_colors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
			free(is_vertex);
			free(*scc_id);
			free(colors);
			return 0;
		}
		size_t n_colors = 0;

		// from the way colors was initialized, the unique colors are 
		// those of the vertices v such that colors[v] = v, then c := v.
		// we append c to the unique_colors array
		for(size_t v = 0 ; v < G->n_verts ; ++v) {
			if(is_vertex[v] && colors[v] == v) 
				unique_colors[n_colors++] = v;
		}

		// free the extra memory allocated to unique_colors
		unique_colors = (size_t *) realloc(unique_colors, n_colors * sizeof(size_t));

		// then loop over all the unique colors c
		for(size_t i = 0 ; i < n_colors ; ++i) {
			size_t c = unique_colors[i];

			// perform a backward bfs on the subgraph of G where colors[v] = c
			// these create a new scc
			size_t *scc_c;
			size_t n_scc_c = backward_bfs(c, G, c, colors, is_vertex, &scc_c);

			if(n_scc_c > 0) {
				// for each vertex in the new scc set scc_id = c and increase n_scc
				for(size_t j = 0 ; j < n_scc_c ; ++j) {
					size_t v = scc_c[j];
					(*scc_id)[v] = c;

					// finally remove the vertices from the graph
					is_vertex[v] = 0;
					n_active_verts -= 1;
				}
				n_scc += 1;


				free(scc_c);
			}
		}

		free(unique_colors);
		free(colors);
	}

	free(is_vertex);

	return n_scc;
}
