/* graph struct methods
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

#include "graph.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>
#include <string.h>

#include <mmio.h>

/* Initialize a graph struct in the CSC and CSR format.
 *
 * takes as input a pointer to the struct to be initialized and the number of
 * vertices and edges, then allocates the required memory to row_id and col_id.
 *
 * the matrix struct should be freed by using free_matrix(&csc)
 */
graph *initialize_graph(size_t n_verts, size_t n_edges) {
	graph *G = (graph *) malloc(sizeof(graph));
	if(G == NULL) return NULL;

	// initializing the variables
	G->n_verts = n_verts;
	G->n_edges = n_edges;

	// in CSR format, col_id is of size n_edges and row_id of size n_verts + 1
	G->csr_col_id = (vert_t *) malloc(n_edges * sizeof(vert_t));
	G->csr_row_id = (edge_t *) malloc((n_verts + 1) * sizeof(edge_t));

	// in CSC format, row_id is of size n_edges and col_id of size n_verts + 1
	G->csc_row_id = (vert_t *) malloc(n_edges * sizeof(vert_t));
	G->csc_col_id = (edge_t *) malloc((n_verts + 1) * sizeof(edge_t));

	if (G->csr_col_id == NULL || G->csr_row_id == NULL || G->csc_col_id == NULL || G->csc_row_id == NULL)  {
		return NULL;
	}

	return G;
}

/* Free the memory allocated to a graph struct
 *
 * takes as input a pointer to the struct and frees the memory allocated to row_id and col_id.
 */
void free_graph(graph *G) {
	free(G->csr_col_id);
	free(G->csr_row_id);

	free(G->csc_col_id);
	free(G->csc_row_id);

	free(G);
}


/* Gets the neighbours of vertex in graph G.
 *
 * neighbours refers to the vertices u such that there exists an edge (v, u) in graph G
 * takes as input the vertex, a pointer to the graph and a pointer to the array of vertices, 
 * as well as an array of vertices that will be considered as active vertices on the graph.
 * allocates and initializes the array of neighbours and returns the number of neighbours.
 */
ssize_t get_neighbours(vert_t vertex, const graph *G, const bool *is_vertex, vert_t **neighbours) {
	// checks if the vertex is contained in the graph
	if(!is_vertex[vertex]) {
		return 0;
	}

	// in the CSR format the vertices u that a given vertex v point to are given
	// in the col_id array at indices row_id[v]..row_id[v+1]
	edge_t col_index_start = G->csr_row_id[vertex];
	edge_t col_index_end = G->csr_row_id[vertex + 1];
	size_t neighbour_count = col_index_end - col_index_start;

	// checks if there are any neighbours
	if(neighbour_count > 0) {
		// allocates the required memory and handles for errors
		*neighbours = (vert_t *) malloc(neighbour_count * sizeof(vert_t));
		if(*neighbours == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));
			return -1;
		}

		// then fills the array with the correct data
		size_t j = 0;
		for(size_t i = 0; i < neighbour_count ; ++i) {
			vert_t vertex_to_add = G->csr_col_id[col_index_start + i];
			if(is_vertex[vertex_to_add]) {
				(*neighbours)[j++] = vertex_to_add;
			}
		}

		neighbour_count = j;
		*neighbours = (vert_t *) realloc(*neighbours, neighbour_count * sizeof(vert_t));
	}

	return neighbour_count;
}

/* Gets the predecessors of vertex in graph G.
 *
 * predecessor refers to the vertices u such that there exists an edge (u, v) in graph G
 * takes as input the vertex, a pointer to the graph and a pointer to the array of vertices,
 * as well as an array of vertices that will be considered as active vertices on the graph.
 * allocates and initializes the array of predecessors and returns the number of predecessors.
 */
ssize_t get_predecessors(vert_t vertex, const graph *G, const bool *is_vertex, vert_t **predecessors) {
	// checks if the vertex is contained in the graph
	if(!is_vertex[vertex]) {
		return 0;
	}

	edge_t row_index_start = G->csc_col_id[vertex];
	edge_t row_index_end = G->csc_col_id[vertex + 1];
	size_t predecessor_count = row_index_end - row_index_start;

	// checks if there are any predecessors
	if(predecessor_count > 0) {
		// allocates the required memory and handles for errors
		*predecessors = (vert_t *) malloc(predecessor_count * sizeof(vert_t));
		if(*predecessors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));
			return -1;
		}

		// then fills the array with the correct data
		size_t j = 0;
		for(size_t i = 0 ; i < predecessor_count ; ++i) {
			vert_t vertex_to_add = G->csc_row_id[row_index_start + i];
			if(is_vertex[vertex_to_add]) {
				(*predecessors)[j++] = vertex_to_add;
			}
		}

		predecessor_count = j;
		*predecessors = (vert_t *) realloc(*predecessors, predecessor_count * sizeof(vert_t));
	}

	return predecessor_count;
}


/* Performs BFS on graph G starting from start_vertex on nodes that 
 * have search_property and saves the result in search_result
 *
 * properties must be of size n_verts and contains the property value for all the vertices.
 * the method will result in all verts that can be reached from
 * start_vertex using bfs via transfer and are homogeneous in search_property,
 * meaning it will only search the subgraph G' with the nodes that have search_property.
 *
 * saves the result in search_result returns the size of search_result
 */
ssize_t bfs(
		vert_t start_vertex, const graph *G, 
		ssize_t (*transfer)(vert_t, const graph *, const bool *, vert_t **), 
		vert_t search_property, const vert_t *properties, const bool *is_vertex, 
		vert_t **search_result) {

	if(!is_vertex[start_vertex] || properties[start_vertex] != search_property) return 0;

	// initialize visited array. this will contain all vertices visited
	bool *visited = (bool *) malloc(G->n_verts * sizeof(bool));
	if(visited == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));
		return -1;
	}

	for(vert_t i = 0 ; i < G->n_verts ; ++i) visited[i] = false;
	visited[start_vertex] = true;

	// the vertex queue will contain all the vertices that have to be explored
	// it will contain at most n_active_verts. head and tail index the head and
	// tail position of the queue
	vert_t *vertex_queue = (vert_t *) malloc(G->n_verts * sizeof(vert_t));
	if(vertex_queue == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));

		free(visited);
		return -1;
	}
	vert_t head = 0;
	vert_t tail = 0;

	// enqueue start_vertex
	vertex_queue[tail++] = start_vertex;

	// while the queue is not empty
	while(tail > head) {
		// dequeue v
		vert_t v = vertex_queue[head++];

		// get all the vertices that are reachable from v through transfer
		vert_t *front;
		ssize_t front_size = (*transfer)(v, G, is_vertex, &front);
		if(front_size == -1) {
			free(visited);
			free(vertex_queue);
			return -1;
		} else if(front_size > 0) {
			// for each w reachable from v
			for(size_t i = 0 ; i < front_size ; ++i) {
				vert_t w = front[i];
				
				// if w not visited and w has search_property
				if (!visited[w] && properties[w] == search_property) {
					// mark w as visited
					visited[w] = true;
					
					// enqueue w
					vertex_queue[tail++] = w;
				}
			}

			free(front);
		}
	}

	free(visited);

	size_t n_visited = tail;

	vertex_queue = (vert_t *) realloc(vertex_queue, n_visited * sizeof(vert_t));
	*search_result = vertex_queue;

	return n_visited;
}

// Performs BFS on graph G with transfer=get_neighbours
ssize_t forward_bfs(
		vert_t start_vertex, const graph *G, 
		vert_t search_property, const vert_t *properties, const bool *is_vertex,
		vert_t **search_result) {
	return bfs(start_vertex, G, get_neighbours, search_property, properties, is_vertex, search_result);
}

// Performs BFS on graph G with transfer=get_predecessors
ssize_t backward_bfs(
		vert_t start_vertex, const graph *G, 
		vert_t search_property, const vert_t *properties, const bool *is_vertex,
		vert_t **search_result) {
	return bfs(start_vertex, G, get_predecessors, search_property, properties, is_vertex, search_result);
}


/* Returns true if v is a trivial SCC
 *
 * this is the case if v has no neighbours or no predecessors
 * or if its only neighbour/predecessor is itself
 */
int is_trivial_scc(vert_t v, const graph *G, const bool *is_vertex) {
	vert_t *N;
	ssize_t n_N = get_neighbours(v, G, is_vertex, &N);

	if(n_N == -1) return -1;
	else if(n_N == 0) return 1;
	else if(n_N == 1 && N[0] == v) {
		free(N);
		return 1;
	}

	free(N);

	vert_t *P;
	ssize_t n_P = get_predecessors(v, G, is_vertex, &P);

	if(n_P == -1) return -1;
	else if(n_P == 0) return 1;
	else if(n_P == 1 && P[0] == v) {
		free(P);
		return 1;
	}

	free(P);

	return false;
}


/* Compare the first index given 2 pairs of integer points
 *
 * helper function to be used inside qsort.
 * takes as input a pair a,b of void* that point to pairs of numbers,
 * used for row and column indexes, and compares them based on their first element
 * effectively comparing between their row indices. if they have the same row index,
 * it compares them based on their second element (column index)
 *
 * returns -1 if row_a < row_b or if row_a = row_b and col_a < col_b
 * 			1 if row_a > row_b or if row_a = row_b and col_a > col_b
 * 			0 if row_a = row_b and col_a = col_b
 */
static int comp_row(const void *a, const void *b) {
	vert_t row_a = (*((vert_t **)a))[0];
	vert_t row_b = (*((vert_t **)b))[0];

	vert_t col_a = (*((vert_t **)a))[1];
	vert_t col_b = (*((vert_t **)b))[1];

	if(row_a < row_b) return -1;
	else if(row_a > row_b) return 1;
	else {
		if(col_a < col_b) return -1;
		else if(col_a > col_b) return 1;
		else return 0;
	}
}

/* Compare the second index given 2 pairs of integer points
 *
 * helper function to be used inside qsort.
 * takes as input a pair a,b of void* that point to pairs of numbers,
 * used for row and column indexes, and compares them based on their second element
 * effectively comparing between their column indices.
 *
 * returns -1 if col_a < col_b or if col_a = col_b and row_a < row_b
 * 			1 if col_a > col_b or if col_a = col_b and row_a > row_b
 * 			0 if col_a = col_b and row_a = row_b
 */
static int comp_col(const void *a, const void *b) {
	vert_t row_a = (*((vert_t **)a))[0];
	vert_t row_b = (*((vert_t **)b))[0];

	vert_t col_a = (*((vert_t **)a))[1];
	vert_t col_b = (*((vert_t **)b))[1];
	
	if(col_a < col_b) return -1;
	else if(col_a > col_b) return 1;
	else {
		if(row_a < row_b) return -1;
		else if(row_a > row_b) return 1;
		else return 0;
	}
}

/* Imports a graph's adj. matrix from a MatrixMarket .mtx file and stores it in a graph struct
 *
 * Takes as input the path to the .mtx file to be imported and a pointer to the graph struct.
 *
 * It initializes the struct based on the size of the matrix and fills its
 * arrays with the correct values.
 *
 * The .mtx file must be in the format sparse (coordinate) and general, and the values must
 * be either integer, pattern or real.
 *
 * we are concerned mostly with the shape of the graph the matrix represents so the
 * values are discarded, and only the location of the nonzero elements is saved.
 */
graph *import_graph(char *mtx_fname) {

	// Attempt to open the file mtx_fname and checking for errors.
	FILE *mtx_file = NULL;
	mtx_file = fopen(mtx_fname, "r"); if(mtx_file == NULL) { 
		fprintf(stderr, "Error opening file: %s\n%s\n", mtx_fname, strerror(errno));
		return NULL;
	}

	// The typecode struct stores information about the type of matrix the .mtx file represents
	MM_typecode mtx_type;

	// Attempt to read the banner of the matrix, handle errors
	int mm_read_err_code = 0;
	mm_read_err_code = mm_read_banner(mtx_file, &mtx_type);
	if(mm_read_err_code) {
		fprintf(stderr, "Error reading MatrixMarket banner: %s\n", mtx_fname);

		switch(mm_read_err_code) {
			case MM_PREMATURE_EOF:
				fprintf(stderr, "Items missing from file header\n");
				break;
			case MM_NO_HEADER:
				fprintf(stderr, "File missing header\n");
				break;
			case MM_UNSUPPORTED_TYPE:
				fprintf(stderr, "Invalid header information\n");
				break;
			default:
				fprintf(stderr, "Unknown error code: %d\n", mm_read_err_code);
				break;
		}

		fclose(mtx_file);
		return NULL;
	}

	// the dimensions of the matrix and the nonzero elements
	size_t n_rows = 0;
	size_t n_cols = 0;
	size_t n_nz = 0;

	// Attempt to read size information, only if matrix is of type coordinate and general
	if(mm_is_coordinate(mtx_type) && mm_is_general(mtx_type)) {
		mm_read_err_code = mm_read_mtx_crd_size(mtx_file, (int *) &n_rows, (int *) &n_cols, (int *) &n_nz);
	} else {
		char* type = mm_typecode_to_str(mtx_type);
		fprintf(stderr, "Invalid matrix type: %s\nmatrix must be of type coordinate and general\n", type);
		free(type);

		fclose(mtx_file);
		return NULL;
	}

	// Handle errors related to reading the size information
	if(mm_read_err_code) {
		fprintf(stderr, "Error reading MatrixMarket matrix size: %s\n", mtx_fname);

		switch(mm_read_err_code) {
			case MM_PREMATURE_EOF:
				fprintf(stderr, "EOF encountered while reading matrix size\n");
				break;
			default:
				fprintf(stderr, "Unknown error code: %d\n", mm_read_err_code);
				break;
		}

		fclose(mtx_file);
		return NULL;
	}

	// graph adjacent matrices are square. fail if n_rows != n_cols
	if(n_rows != n_cols) {
		fprintf(stderr, 
				"Error: incompatible size: %s\nmatrix must have equal number of rows and columns.\n", 
				mtx_fname);

		fclose(mtx_file);
		return NULL;
	}

	// indices will store the pairs of row, column indices of nonzero elements
	// in the .mtx file, later to be stored in the appropriate structs
	vert_t **indices;

	// there are n_nz such pairs of indices
	indices = (vert_t **) malloc(n_nz * sizeof(vert_t *)); if(indices == NULL) {
		fprintf(stderr, "Error allocating memory:\n%s\n", strerror(ENOMEM));

		fclose(mtx_file);
		return NULL;
	}

	for(edge_t i = 0 ; i < n_nz ; ++i) {
		indices[i] = (vert_t *) malloc(2 * sizeof(vert_t));

		vert_t row, col;

		int fscanf_match_needed = 0;
		int fscanf_match_count = 0;
		// get the indices from the file, depending on the type of Matrix in the file,
		// discarding the value of the matrix if needed.
		if(mm_is_pattern(mtx_type)) {
			fscanf_match_needed = 2;
			fscanf_match_count = fscanf(mtx_file, "%u %u\n", &row, &col);
		} else if(mm_is_integer(mtx_type)) {
			int val;
			fscanf_match_needed = 3;
			fscanf_match_count = fscanf(mtx_file, "%u %u %d\n", &row, &col, &val);
		} else if(mm_is_real(mtx_type)) {
			double val;
			fscanf_match_needed = 3;
			fscanf_match_count = fscanf(mtx_file, "%u %u %lf\n", &row, &col, &val);
		} else {
			fprintf(stderr, "MatrixMarket file is of unsupported format: %s\n", mtx_fname);

			for(edge_t j = 0 ; j <= i ; ++j) free(indices[j]);
			free(indices);

			fclose(mtx_file);
			return NULL;
		}

		if(fscanf_match_count == EOF) {
			if(ferror(mtx_file)) {
				fprintf(stderr, "Error: reading from %s\n", mtx_fname);
			} else {
				fprintf(stderr, "Error: fscanf matching failure for %s\n", mtx_fname);
			}
			
			for(edge_t j = 0 ; j <= i ; ++j) free(indices[j]);
			free(indices);

			fclose(mtx_file);
			return NULL;
		} else if(fscanf_match_count < fscanf_match_needed) {
			fprintf(stderr, "Error reading from %s:\nfscanf early matching failure\n", mtx_fname);

			for(edge_t j = 0 ; j <= i ; ++j) free(indices[j]);
			free(indices);

			fclose(mtx_file);
			return NULL;
		}

		if(row > n_rows || col > n_cols) {
			fprintf(stderr, "Invalid index in .mtx file %s\n", mtx_fname);

			for(edge_t j = 0 ; j <= i ; ++j) free(indices[j]);
			free(indices);

			fclose(mtx_file);
			return NULL;
		}

		indices[i][0] = row - 1;
		indices[i][1] = col - 1;
	}
	// since we have read all the data from the .mtx file we can close it.
	fclose(mtx_file);


	// the number of vertices equals the rows of the matrix
	// the number of edges is the number of non zero elements
	size_t n_verts = n_rows;
	size_t n_edges = n_nz;

	// initialize the graph struct and handle errors
	graph *G = NULL;
	if((G = initialize_graph(n_verts, n_edges)) == NULL) {
		fprintf(stderr, "Error initializing CSC matrix: %s\n%s\n", mtx_fname, strerror(ENOMEM));

		for(edge_t j = 0 ; j < n_nz ; ++j) free(indices[j]);
		free(indices);

		return NULL;
	}


	/* the indices array is effectively a COO representation of the matrix.
	 *
	 * the code for converting from COO to CSC/CSR was adapted from the link below:
	 * https://stackoverflow.com/questions/23583975/convert-coo-to-csr-format-in-c
	 */

	// sort the indices based on the column index in order to create the CSC format
	qsort(indices, n_nz, sizeof(vert_t *), comp_col);

	// csc_col_id[col + 1] will initialy hold the number of nz elements in col, 
	// then we perform a cumulative sum which will be in the form we need.
	
	// initialize csc_col_id to 0. 
	for(vert_t i = 0 ; i <= n_verts ; ++i) {
		G->csc_col_id[i] = 0;
	}

	// then we loop over the COO array
	for(edge_t i = 0 ; i < n_edges ; ++i) {
		vert_t row = indices[i][0];
		vert_t col = indices[i][1];

		// store the row in row_id as is
		G->csc_row_id[i] = row;

		// increase col_id for index: col+1 since 1 more nz element is present in col
		G->csc_col_id[col + 1] += 1;
	}

	// then perform the cumulative sum
	for(vert_t i = 0 ; i < n_verts ; ++i) {
		G->csc_col_id[i + 1] += G->csc_col_id[i];
	}


	// then sort the indices based on row index, for the CSR format
	qsort(indices, n_nz, sizeof(vert_t *), comp_row);

	// then initialize the CSR struct in the same way as above, 
	// with col_id and row_id switched.
	
	// initialize csc_row_id to 0.
	for(vert_t i = 0 ; i <= n_verts ; ++i) {
		G->csr_row_id[i] = 0;
	}

	// then we loop over the COO array
	for(edge_t i = 0 ; i < n_edges ; ++i) {
		vert_t row = indices[i][0];
		vert_t col = indices[i][1];

		// store the col in col_id as is
		G->csr_col_id[i] = col;

		// increase row_id for index: row+1 since 1 more nz element is present in row
		G->csr_row_id[row + 1] += 1;
	}

	// then perform the cumulative sum
	for(vert_t i = 0 ; i < n_verts ; ++i) {
		G->csr_row_id[i + 1] += G->csr_row_id[i];
	}

	// finally free the memory in the indices array, since it is no longer needed.
	for(edge_t j = 0 ; j < n_nz ; ++j) free(indices[j]);
	free(indices);

	return G;
}

