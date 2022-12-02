/* OpenCilk scc implementation methods
 * Copyright (C) 2022  Alexandros Athanasiadis
 *
 * This file is part of scc
 *                                                                        
 * scc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *                                                                        
 * scc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *                                                                        
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. 
 */

#include "scc_opencilk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>

#include <string.h>

#include <cilk/cilk.h>


// implements a cilk sum reducer
void sum_identity(void *view) { *(size_t *)view = 0; }
void sum_reducer(void *left, void* right) { *(size_t *)left += *(size_t *)right; }

// implements a cilk or reducer
void or_identity(void *view) { *(bool *)view = false; }
void or_reducer(void *left, void *right) { *(bool *)left = *(bool *)left || *(bool *)right; }

/* Implements the graph coloring algorithm to find the SCCs of G
 *
 * takes as input the graph G and a double pointer where the result will 
 * be stored. returns the number of sccs.
 *
 * scc_id is of size n_verts
 * if v belongs to the scc with id c then: scc_id[v] = c
 */
ssize_t cilk_scc_coloring(const graph *G, vert_t **scc_id) {
	
	bool *is_vertex = (bool *) malloc(G->n_verts * sizeof(bool));
	if(is_vertex == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));
		return -1;
	}
	cilk_for(vert_t v = 0 ; v < G->n_verts ; ++v) is_vertex[v] = true;
	size_t n_active_verts = G->n_verts;

	// allocate the memory required for the scc_id array
	*scc_id = (vert_t *) malloc(G->n_verts * sizeof(vert_t));
	if(*scc_id == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));

		free(is_vertex);
		return -1;
	}

	// initialize n_sccs to 0
	size_t n_scc = 0;
	
	// remove trivial sccs 
	// the loop will run just twice since after that
	// you get diminishing returns
	for(uint8_t i = 0 ; i < 2 ; ++i) {
		size_t cilk_reducer(sum_identity, sum_reducer) verts_removed = 0;

		// loop over all vertices
		cilk_for(vert_t v = 0 ; v < G->n_verts ; ++v) {
			if(is_vertex[v]) {
				int is_trivial = is_trivial_scc(v, G, is_vertex);

				// check if the vertex is active, and then if it is trivial
				if(is_trivial) {
					// if it is, set scc_id for the vertex to be itself
					// and increase the number of sccs
					(*scc_id)[v] = v;

					// finally remove the vertex from the graph
					is_vertex[v] = false;
					verts_removed++;
				}
			}
		}

		n_active_verts -= verts_removed;
		n_scc += verts_removed;
	}

	// the core loop of the algorithm
	// this will run as long as G is non empty
	while(n_active_verts > 0) {
		// initialize the colors array as colors(v) = v for each v in G
		vert_t *colors = (vert_t *) malloc(G->n_verts * sizeof(vert_t));
		if(colors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));

			free(is_vertex);
			free(*scc_id);
			return -1;
		}
		cilk_for(vert_t v = 0 ; v < G->n_verts ; ++v) colors[v] = v;

		// this loop will run as long as at least one vertex changed colors in
		// the last iteration since a vertex changing color might end up changing
		// the color of its neighbours in the next iteration.
		bool cilk_reducer(or_identity, or_reducer) changed_color = true;
		while(changed_color) {
			changed_color = false;

			// we loop over all the vertives v in the graph (checking if the v is active)
			cilk_for(vert_t v = 0 ; v < G->n_verts ; ++v) {
				if(is_vertex[v]) {
					// we get the predecessors of the vertex v (vertices u such that [u, v] in G)
					// because we want to write in one memory position (colors[v])
					// as opposed to every u for each v. this is useful for 
					// the parallelization since memory locations the treads
					// write to will not interfere.

					vert_t *predecessors;
					ssize_t n_predecessors = get_predecessors(v, G, is_vertex, &predecessors);
					if(n_predecessors > 0) {
						// then we set colors[v] to be the minimum of its predecessors (or itself)
						for(size_t i = 0 ; i < n_predecessors ; ++i) {
							vert_t u = predecessors[i];
							if(colors[v] > colors[u]) {
								colors[v] = colors[u];
								changed_color = true;
							}
						}

						free(predecessors);
					}

				}
			}
		}


		// after the coloring is finished we need to find all the unique colors c in the colors array
		// there may be up to n_verts unique colors (one for each vertex)
		vert_t *unique_colors = (vert_t *) malloc(G->n_verts * sizeof(vert_t));
		if(unique_colors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));

			free(is_vertex);
			free(*scc_id);
			free(colors);

			return -1;
		}
		size_t n_colors = 0;

		// from the way colors was initialized, the unique colors are 
		// those of the vertices v such that colors[v] = v, then c := v.
		// we append c to the unique_colors array
		for(vert_t v = 0 ; v < G->n_verts ; ++v) {
			if(is_vertex[v] && colors[v] == v) 
				unique_colors[n_colors++] = v;
		}

		// free the extra memory allocated to unique_colors
		unique_colors = (vert_t *) realloc(unique_colors, n_colors * sizeof(vert_t));

		size_t cilk_reducer(sum_identity, sum_reducer) verts_removed = 0;
		size_t cilk_reducer(sum_identity, sum_reducer) sccs_found_thd = 0;

		// then loop over all the unique colors c
		cilk_for(size_t i = 0 ; i < n_colors ; ++i) {
			vert_t c = unique_colors[i];

			// perform a backward bfs on the subgraph of G where colors[v] = c
			// these create a new scc
			vert_t *scc_c;
			ssize_t n_scc_c = backward_bfs(c, G, c, colors, is_vertex, &scc_c);
			if(n_scc_c > 0) {
				// for each vertex in the new scc set scc_id = c and increase n_scc
				for(size_t j = 0 ; j < n_scc_c ; ++j) {
					vert_t v = scc_c[j];
					(*scc_id)[v] = c;

					// finally remove the vertices from the graph
					is_vertex[v] = false;
				}

				verts_removed += n_scc_c;
				sccs_found_thd += 1;

				free(scc_c);
			}
		}

		n_active_verts -= verts_removed;
		n_scc += sccs_found_thd;

		free(unique_colors);
		free(colors);
	}

	free(is_vertex);

	return n_scc;
}
