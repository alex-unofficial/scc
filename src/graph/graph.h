/* graph struct header
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

#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// these types will be used for indexing vertices and edges respectively.
typedef uint32_t vert_t;
typedef uint32_t edge_t;

/* graph is a struct used to represent graphs, by storing their adjacency matrices as
 * sparse pattern matrices, in the CSR and CSC formats.
 *
 * the CSR and CSC formats store only the nonzero elements of the sparse matrix.
 * in the case of CSC, row_id stores the row indices of the nonzero elements
 * while col_id stores the position in the row_id array where the nonzero elements
 * of the column begins. the opposite is true for CSR.
 *
 * since this is a pattern matrix, we dont store the value of nonzero elements,
 * but only their position in the matrix. this means that the graph
 * will only encode directionality, not edge weights.
 */
typedef struct graph{
	size_t n_verts;
 	size_t n_edges;

	edge_t *csr_row_id;
	vert_t *csr_col_id;

	vert_t *csc_row_id;
	edge_t *csc_col_id;

} graph;

/* initialization and free functions */

// Initialize a graph struct in the CSC and CSR format.
graph *initialize_graph(size_t n_verts, size_t n_edges);

// Free the memory allocated to a graph struct
void free_graph(graph *G);


/* transfer functions */

// Gets the neighbours of vertex in graph G.
ssize_t get_neighbours(vert_t vertex, const graph *G, const bool *is_vertex, vert_t **neighbours);

// Gets the predecessors of vertex in graph G.
ssize_t get_predecessors(vert_t vertex, const graph *G, const bool *is_vertex, vert_t **predecesors);


/* BFS functions */

// Performs BFS on graph G starting from start_vertex on nodes that 
// have search_property and saves the result in search_result
ssize_t bfs(
		vert_t start_vertex, const graph *G, 
		ssize_t (*transfer)(vert_t, const graph *, const bool *, vert_t **), 
		vert_t search_property, const vert_t *properties, const bool *is_vertex, 
		vert_t **search_result);

// Performs BFS on graph G with transfer=get_neighbours
ssize_t forward_bfs(
		vert_t start_vertex, const graph *G, 
		vert_t search_property, const vert_t *properties, const bool *is_vertex, 
		vert_t **search_result);

// Performs BFS on graph G with transfer=get_predecessors
ssize_t backward_bfs(
		vert_t start_vertex, const graph *G, 
		vert_t search_property, const vert_t *properties, const bool *is_vertex, 
		vert_t **search_result);


/* SCC helper functions */

// Returns true if v is a trivial SCC
int is_trivial_scc(vert_t v, const graph *G, const bool *is_vertex);


/* graph import function */

// Imports a graph's adj. matrix from a MatrixMarket .mtx file and stores it in a graph struct
graph *import_graph(char *mtx_fname);

#endif
