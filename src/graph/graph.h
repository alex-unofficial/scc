#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>

/* graph is a struct used to represent graphs, by storing their adjacency matrices as
 * sparse pattern matrices, in the CSR and CSC formats.
 *
 * the CSR and CSC formats store only the nonzero elements of the sparse matrix.
 * in the case of CSR, row_id stores the row indices of the nonzero elements
 * while col_id stores the position in the row_id array where the nonzero elements
 * of the column begins. the opposite is true for CSC.
 *
 * since this is a pattern matrix, we dont store the value of nonzero elements,
 * but only their position in the matrix. this means that the graph
 * will only encode directionality, not edge weights.
 */
typedef struct graph{
	size_t n_verts;
	size_t n_edges;

	size_t *csr_row_id;
	size_t *csr_col_id;

	size_t *csc_row_id;
	size_t *csc_col_id;
} graph;

/* initialization and free functions */

// Initialize a graph struct in the CSC and CSR format.
int initialize_graph(graph *G, size_t n_verts, size_t n_edges);

// Free the memory allocated to a graph struct
void free_graph(graph *G);


/* transfer functions */

// Gets the neighbours of vertex in graph G.
size_t get_neighbours(size_t vertex, const graph *G, const size_t *is_vertex, size_t **neighbours);

// Gets the predecessors of vertex in graph G.
size_t get_predecessors(size_t vertex, const graph *G, const size_t *is_vertex, size_t **predecesors);


/* BFS functions */

// Performs BFS on graph G starting from start_vertex on nodes that 
// have search_property and saves the result in search_result
size_t bfs(
		size_t start_vertex, const graph *G, 
		size_t (*transfer)(size_t, const graph *, const size_t *is_vertex, size_t **), 
		size_t search_property, const size_t *properties, const size_t *is_vertex, 
		size_t **search_result);

// Performs BFS on graph G with transfer=get_neighbours
size_t forward_bfs(
		size_t start_vertex, const graph *G, 
		size_t search_property, const size_t *properties, const size_t *is_vertex, 
		size_t **search_result);

// Performs BFS on graph G with transfer=get_predecessors
size_t backward_bfs(
		size_t start_vertex, const graph *G, 
		size_t search_property, const size_t *properties, const size_t *is_vertex, 
		size_t **search_result);


/* graph import function */

// Imports a graph's adj. matrix from a MatrixMarket .mtx file and stores it in a graph struct
int import_graph(char *mtx_fname, graph *G);

#endif
