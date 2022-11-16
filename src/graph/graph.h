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

	size_t n_active_verts;
	size_t *vertex_active;
} graph;

// Initialize a graph struct in the CSC and CSR format.
int initialize_graph(graph *G, size_t n_verts, size_t n_edges);

// Free the memory allocated to a graph struct
void free_graph(graph *G);

// Checks if vertex is a valid and active vertex in G
int is_vertex(size_t vertex, graph *G);

// Returns all the active vertices in the graph
size_t get_vertices(graph *G, size_t **vertices);

// Removes vertex from graph G
void remove_vertex(size_t vertex, graph *G);

// Removes every vertex in vertices from graph G
void remove_vertices(size_t *vertices, size_t vertex_count, graph *G);

// Gets the neighbours of vertex in graph G.
size_t get_neighbours(size_t vertex, graph *G, size_t **neighbours);

// Gets the predecessors of vertex in graph G.
size_t get_predecessors(size_t vertex, graph *G, size_t **predecesors);

// Imports a graph's adj. matrix from a MatrixMarket .mtx file and stores it in a graph struct
int import_graph(char *mtx_fname, graph *G);

#endif
