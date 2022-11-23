#include "scc_pthreads.h"

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <string.h>


#define NUMTHREADS 4


struct init_vertex_args {
	size_t start;
	size_t end;
	bool *is_vertex;

}; static void *p_init_is_vertex(void *args) {
	struct init_vertex_args *ivargs = (struct init_vertex_args *) args;
	for(size_t v = ivargs->start ; v < ivargs->end ; ++v) {
		ivargs->is_vertex[v] = true;
	}
	return NULL;
} 


struct init_colors_args {
	size_t start;
	size_t end;
	size_t *colors;

}; static void *p_init_colors(void *args) {
	struct init_colors_args *icargs = (struct init_colors_args *) args;
	for(size_t v = icargs->start ; v < icargs->end ; ++v) {
		icargs->colors[v] = v;
	}
	return NULL;
}


struct init_unique_colors_args {
	size_t start;
	size_t end;

	bool *is_vertex;
	size_t *colors;

	size_t *unique_colors;
	size_t *n_colors;

	pthread_mutex_t *n_colors_lock;

}; static void *p_init_unique_colors(void *args) {
	struct init_unique_colors_args *iucargs = (struct init_unique_colors_args *)args;

	// from the way colors was initialized, the unique colors are 
	// those of the vertices v such that colors[v] = v, then c := v.
	// we append c to the unique_colors array
	for(size_t v = iucargs->start ; v < iucargs->end ; ++v) {
		if(iucargs->is_vertex[v] && iucargs->colors[v] == v) {
			pthread_mutex_lock(iucargs->n_colors_lock);
			iucargs->unique_colors[(*(iucargs->n_colors))++] = v;
			pthread_mutex_unlock(iucargs->n_colors_lock);
		}
	}
}


struct trimming_args {
	size_t start;
	size_t end;

	const graph *G;
	bool *is_vertex;

	bool *removed_vertex;

	size_t **scc_id;
	size_t n_scc_t;

}; static void *p_trimming(void *args) {
	struct trimming_args *trargs = (struct trimming_args *) args;

	trargs->n_scc_t = 0;

	// loop over all vertices between start and end
	for(size_t v = trargs->start ; v < trargs->end ; ++v) {
		// check if the vertex is active, and then if it is trivial
		if(trargs->is_vertex[v] && is_trivial_scc(v, trargs->G, trargs->is_vertex)) {
			// if it is, set scc_id for the vertex to be itself
			// and increase the number of sccs
			(*(trargs->scc_id))[v] = v;
			trargs->n_scc_t += 1;

			// finally remove the vertex from the graph
			trargs->is_vertex[v] = false;
			*(trargs->removed_vertex) = true;
		}
	}

	return NULL;
}

struct coloring_args {
	size_t start;
	size_t end;

	const graph *G;
	bool *is_vertex;

	bool *changed_color;

	size_t *colors;

}; static void *p_coloring(void *args) {
	struct coloring_args *colargs = (struct coloring_args *) args;

	// we loop over all the vertives v in the graph between start and end (checking if v is active)
	for(size_t v = colargs->start ; v < colargs->end ; ++v) {
		if(colargs->is_vertex[v]) {
			// we get the predecessors of the vertex v (vertices u such that [u, v] in G)
			// because we want to write in one memory position (colors[v])
			// as opposed to every u for each v. this is useful for 
			// the parallelization since memory locations the treads
			// write to will not interfere.

			size_t *predecessors;
			size_t n_predecessors = get_predecessors(v, colargs->G, colargs->is_vertex, &predecessors);

			if(n_predecessors > 0) {
				// then we set colors[v] to be the minimum of its predecessors (or itself)
				for(size_t i = 0 ; i < n_predecessors ; ++i) {
					size_t u = predecessors[i];
					if(colargs->colors[v] > colargs->colors[u]) {
						colargs->colors[v] = colargs->colors[u];
						*(colargs->changed_color) = true;
					}
				}

				free(predecessors);
			}

		}
	}

	return NULL;
}

struct get_sccs_args {
	size_t start;
	size_t end;

	const graph *G;
	bool *is_vertex;

	size_t *colors;
	size_t *unique_colors;

	size_t **scc_id;
	
	size_t n_scc_t;
	size_t n_vert_removed;

}; static void *p_get_sccs(void *args) {
	struct get_sccs_args *sccargs = (struct get_sccs_args *) args;

	sccargs->n_scc_t = 0;
	sccargs->n_vert_removed = 0;

	// then loop over all the unique colors c between start and end
	for(size_t i = sccargs->start ; i < sccargs->end ; ++i) {
		size_t c = sccargs->unique_colors[i];

		// perform a backward bfs on the subgraph of G where colors[v] = c
		// these create a new scc
		size_t *scc_c;
		size_t n_scc_c = backward_bfs(c, sccargs->G, c, sccargs->colors, sccargs->is_vertex, &scc_c);

		if(n_scc_c > 0) {
			// for each vertex in the new scc set scc_id = c and increase n_scc
			for(size_t j = 0 ; j < n_scc_c ; ++j) {
				size_t v = scc_c[j];
				(*(sccargs->scc_id))[v] = c;

				// finally remove the vertices from the graph
				sccargs->is_vertex[v] = false;
			}

			sccargs->n_vert_removed += n_scc_c;
			sccargs->n_scc_t += 1;

			free(scc_c);
		}
	}
}

/* Implements the graph coloring algorithm to find the SCCs of G
 *
 * takes as input the graph G and a double pointer where the result will 
 * be stored. returns the number of sccs.
 *
 * scc_id is of size n_verts
 * if v belongs to the scc with id c then: scc_id[v] = c
 */
size_t p_scc_coloring(const graph *G, size_t **scc_id) {

	pthread_t threads[NUMTHREADS];
	const size_t p_block_size = G->n_verts / NUMTHREADS;
	
	bool *is_vertex = (bool *) malloc(G->n_verts * sizeof(bool));
	if(is_vertex == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
		return 0;
	}

	// initialize the is_vertex array in parallel
	struct init_vertex_args ivargs[NUMTHREADS];
	for(int i = 0 ; i < NUMTHREADS ; ++i) {
		ivargs[i].start = i * p_block_size;
		ivargs[i].end = (i == NUMTHREADS - 1)? G->n_verts : (i + 1) * p_block_size;
		ivargs[i].is_vertex = is_vertex;

		pthread_create(&threads[i], NULL, p_init_is_vertex, &ivargs[i]);
	} for(int i = 0 ; i < NUMTHREADS ; ++i) pthread_join(threads[i], NULL);
	

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
	bool removed_vertex = true;
	while(removed_vertex) {
		removed_vertex = false;

		// perform one trimming iteration in parallel
		struct trimming_args trargs[NUMTHREADS];
		for(int i = 0 ; i < NUMTHREADS ; ++i) {
			trargs[i].start = i * p_block_size;
			trargs[i].end = (i == NUMTHREADS - 1)? G->n_verts : (i + 1) * p_block_size;

			trargs[i].G = G;
			trargs[i].is_vertex = is_vertex;

			trargs[i].removed_vertex = &removed_vertex;
			trargs[i].scc_id = scc_id;
			
			pthread_create(&threads[i], NULL, p_trimming, &trargs[i]);
		} for(int i = 0 ; i < NUMTHREADS ; ++i) {
			pthread_join(threads[i], NULL);

			n_scc += trargs[i].n_scc_t;
			n_active_verts -= trargs[i].n_scc_t;
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

		// initializing the colors array in parallel
		struct init_colors_args icargs[NUMTHREADS];
		for(int i = 0 ; i < NUMTHREADS ; ++i) {
			icargs[i].start = i * p_block_size;
			icargs[i].end = (i == NUMTHREADS - 1)? G->n_verts : (i + 1) * p_block_size;
			icargs[i].colors = colors;
			pthread_create(&threads[i], NULL, p_init_colors, &icargs[i]);

		} for(int i = 0 ; i < NUMTHREADS ; ++i) pthread_join(threads[i], NULL);

		// this loop will run as long as at least one vertex changed colors in
		// the last iteration since a vertex changing color might end up changing
		// the color of its neighbours in the next iteration.
		bool changed_color = true;
		while(changed_color) {
			changed_color = false;

			struct coloring_args colargs[NUMTHREADS];
			for(int i = 0 ; i < NUMTHREADS ; ++i) {
				colargs[i].start = i * p_block_size;
				colargs[i].end = (i == NUMTHREADS - 1)? G->n_verts : (i + 1) * p_block_size;

				colargs[i].G = G;
				colargs[i].is_vertex = is_vertex;

				colargs[i].changed_color = &changed_color;

				colargs[i].colors = colors;

				pthread_create(&threads[i], NULL, p_coloring, &colargs[i]);
			} for(int i = 0 ; i < NUMTHREADS ; ++i) pthread_join(threads[i], NULL);
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
		pthread_mutex_t n_colors_lock = PTHREAD_MUTEX_INITIALIZER;

		// initializing the unique colors array in parallel
		struct init_unique_colors_args iucargs[NUMTHREADS];
		for(int i = 0 ; i < NUMTHREADS ; ++i) {
			iucargs[i].start = i * p_block_size;
			iucargs[i].end = (i == NUMTHREADS - 1)? G->n_verts : (i + 1) * p_block_size;

			iucargs[i].is_vertex = is_vertex;
			iucargs[i].colors = colors;

			iucargs[i].unique_colors = unique_colors;
			iucargs[i].n_colors = &n_colors;

			iucargs[i].n_colors_lock = &n_colors_lock;

			pthread_create(&threads[i], NULL, p_init_unique_colors, &iucargs[i]);
		} for(int i = 0 ; i < NUMTHREADS ; ++i) pthread_join(threads[i], NULL);
		

		// free the extra memory allocated to unique_colors
		unique_colors = (size_t *) realloc(unique_colors, n_colors * sizeof(size_t));

		size_t p_color_block_size = n_colors / NUMTHREADS;

		struct get_sccs_args sccargs[NUMTHREADS];
		for(int i = 0 ; i < NUMTHREADS ; ++i) {
			sccargs[i].start = i * p_color_block_size;
			sccargs[i].end = (i == NUMTHREADS - 1)? n_colors : (i + 1) * p_color_block_size;

			sccargs[i].G = G;
			sccargs[i].is_vertex = is_vertex;

			sccargs[i].colors = colors;
			sccargs[i].unique_colors = unique_colors;

			sccargs[i].scc_id = scc_id;
			pthread_create(&threads[i], NULL, p_get_sccs, &sccargs[i]);
		} for (int i = 0 ; i < NUMTHREADS ; ++i) {
			pthread_join(threads[i], NULL);
			
			n_scc += sccargs[i].n_scc_t;
			n_active_verts -= sccargs[i].n_vert_removed;
		}

		free(unique_colors);
		free(colors);
	}

	free(is_vertex);

	return n_scc;
}
