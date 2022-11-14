#include "graph.h"

#include <stdio.h>
#include <stdlib.h>

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
int initialize_graph(graph *G, size_t n_verts, size_t n_edges) {
	// initializing the variables
	G->n_verts = n_verts;
	G->n_edges = n_edges;

	// in CSR format, col_id is of size n_edges and row_id of size n_verts + 1
	G->csr_col_id = (size_t *) malloc(n_edges * sizeof(size_t));
	G->csr_row_id = (size_t *) malloc((n_verts + 1) * sizeof(size_t));

	// in CSC format, row_id is of size n_edges and col_id of size n_verts + 1
	G->csc_row_id = (size_t *) malloc(n_edges * sizeof(size_t));
	G->csc_col_id = (size_t *) malloc((n_verts + 1) * sizeof(size_t));

	if(G->csr_col_id == NULL || G->csr_row_id == NULL || G->csc_col_id == NULL || G->csc_row_id == NULL) {
		return errno;
	}

	return 0;
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
int comp_row(const void *a, const void *b) {
	size_t row_a = (*(size_t**)a)[0];
	size_t row_b = (*(size_t**)b)[0];

	size_t col_a = (*(size_t**)a)[1];
	size_t col_b = (*(size_t**)b)[1];

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
int comp_col(const void *a, const void *b) {
	size_t row_a = (*(size_t**)a)[0];
	size_t row_b = (*(size_t**)b)[0];

	size_t col_a = (*(size_t**)a)[1];
	size_t col_b = (*(size_t**)b)[1];
	
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
int import_graph(char *mtx_fname, graph *G) {

	// Attempt to open the file mtx_fname and checking for errors.
	FILE *mtx_file = NULL;
	mtx_file = fopen(mtx_fname, "r"); if(mtx_file == NULL) { 
		int err = errno;
		fprintf(stderr, "Error opening file: %s\n%s\n", mtx_fname, strerror(err));
		return -1;
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
				fprintf(stderr, "Items missing from file header\n\t");
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
		return -1;
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
		return -1;
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
		return -1;
	}

	// graph adjacent matrices are square. fail if n_rows != n_cols
	if(n_rows != n_cols) {
		fprintf(stderr, 
				"Error: incompatible size: %s\nmatrix must have equal number of rows and columns.\n", 
				mtx_fname);

		fclose(mtx_file);
		return -1;
	}

	// indices will store the pairs of row, column indices of nonzero elements
	// in the .mtx file, later to be stored in the appropriate structs
	size_t** indices;

	// there are n_nz such pairs of indices
	indices = (size_t **) malloc(n_nz * sizeof(size_t *));
	for(size_t i = 0 ; i < n_nz ; ++i) {
		indices[i] = (size_t *) malloc(2 * sizeof(size_t));

		size_t row, col;

		size_t fscanf_match_count = 0;
		// get the indices from the file, depending on the type of Matrix in the file,
		// discarding the value of the matrix if needed.
		if(mm_is_pattern(mtx_type)) {
			fscanf_match_count = fscanf(mtx_file, "%zu %zu\n", &row, &col);
		} else if(mm_is_integer(mtx_type)) {
			int val;
			fscanf_match_count = fscanf(mtx_file, "%zu %zu %d\n", &row, &col, &val);
		} else if(mm_is_real(mtx_type)) {
			double val;
			fscanf_match_count = fscanf(mtx_file, "%zu %zu %f\n", &row, &col, &val);
		} else {
			fprintf(stderr, "MatrixMarket file is of unsupported format: %s\n", mtx_fname);

			for(size_t j = 0 ; j <= i ; ++j) {
				free(indices[j]);
			}
			free(indices);

			fclose(mtx_file);
			return -1;
		}

		if(fscanf_match_count == EOF) {
			int fscanf_err_code = -1;

			if(ferror(mtx_file)) {
				fscanf_err_code = errno;
				fprintf(stderr, "Error reading from %s:\n%s\n", mtx_fname, strerror(fscanf_err_code));
			} else {
				fprintf(stderr, "Error reading from %s:\nfscanf matching failure\n", mtx_fname);
			}
			
			for(size_t j = 0 ; j <= i ; ++j) {
				free(indices[j]);
			}
			free(indices);

			fclose(mtx_file);
			return -1;
		} else if(fscanf_match_count == 0) {
			fprintf(stderr, "Error reading from %s:\nfscanf early matching failure\n", mtx_fname);

			for(size_t j = 0 ; j <= i ; ++j) {
				free(indices[j]);
			}
			free(indices);

			fclose(mtx_file);
			return -1;
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
	int mtx_init_err_code = 0;
	mtx_init_err_code = initialize_graph(G, n_verts, n_edges);
	if(mtx_init_err_code) {
		fprintf(stderr, "Error initializing CSC matrix: %s\n%s\n", mtx_fname, strerror(mtx_init_err_code));

		for(size_t j = 0 ; j < n_nz ; ++j) {
			free(indices[j]);
		}
		free(indices);

		return -1;
	}


	/* the indices array is effectively a COO representation of the matrix.
	 *
	 * the code for converting from COO to CSC/CSR was adapted from the link below:
	 * https://stackoverflow.com/questions/23583975/convert-coo-to-csr-format-in-c
	 */

	// sort the indices based on the column index in order to create the CSC format
	qsort(indices, n_nz, sizeof(size_t *), &comp_col);

	// csc_col_id[col + 1] will initialy hold the number of nz elements in col, 
	// then we perform a cumulative sum which will be in the form we need.
	
	// initialize csc_col_id to 0. 
	for(size_t i = 0 ; i <= n_verts ; ++i) {
		G->csc_col_id[i] = 0;
	}

	// then we loop over the COO array
	for(size_t i = 0 ; i < n_edges ; ++i) {
		size_t row = indices[i][0];
		size_t col = indices[i][1];

		// store the row in row_id as is
		G->csc_row_id[i] = row;

		// increase col_id for index: col+1 since 1 more nz element is present in col
		G->csc_col_id[col + 1] += 1;
	}

	// then perform the cumulative sum
	for(size_t i = 0 ; i < n_verts ; ++i) {
		G->csc_col_id[i + 1] += G->csc_col_id[i];
	}


	// then sort the indices based on row index, for the CSR format
	qsort(indices, n_nz, sizeof(size_t *), &comp_row);

	// then initialize the CSR struct in the same way as above, 
	// with col_id and row_id switched.
	
	// initialize csc_row_id to 0.
	for(size_t i = 0 ; i <= n_verts ; ++i) {
		G->csr_row_id[i] = 0;
	}

	// then we loop over the COO array
	for(size_t i = 0 ; i < n_edges ; ++i) {
		size_t row = indices[i][0];
		size_t col = indices[i][1];

		// store the col in col_id as is
		G->csr_col_id[i] = col;

		// increase row_id for index: row+1 since 1 more nz element is present in row
		G->csr_row_id[row + 1] += 1;
	}

	// then perform the cumulative sum
	for(size_t i = 0 ; i < n_verts ; ++i) {
		G->csr_row_id[i + 1] += G->csr_row_id[i];
	}

	// finally free the memory in the indices array, since it is no longer needed.
	for(size_t i = 0 ; i < n_nz ; ++i) {
		free(indices[i]);
	}
	free(indices);

	return 0;
}

/* Gets the neighbours of vertex in graph G.
 *
 * neighbours refers to the vertices u such that there exists an edge (v, u) in graph G
 * takes as input the vertex, a pointer to the graph and a pointer to the array of vertices.
 * allocates and initializes the array of neighbours and returns the number of neighbours.
 */
size_t get_neighbours(size_t vertex, graph *G, size_t **neighbours) {
	// checks if the vertex is contained in the graph
	if(vertex >= G->n_verts || vertex < 0) {
		fprintf(stderr, "Error: vertex out of bounds\nvertex must be < n_vertex\n");
		return 0;
	}

	// in the CSR format the vertices u that a given vertex v point to are given
	// in the col_id array at indices row_id[v]..row_id[v+1]
	size_t col_index_start = G->csr_row_id[vertex];
	size_t col_index_end = G->csr_row_id[vertex + 1];
	size_t neighbour_count = col_index_end - col_index_start;

	// checks if there are even any neighbours
	if(neighbour_count > 0) {
		// allocates the required memory and handles for errors
		*neighbours = (size_t *) malloc(neighbour_count * sizeof(size_t));
		if(*neighbours == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
			return 0;
		}

		// then fills the array with the correct data
		for(size_t i = 0; i < neighbour_count ; ++i) {
			(*neighbours)[i] = G->csr_col_id[col_index_start + i];
		}
	}

	return neighbour_count;
}

/* Gets the predecessors of vertex in graph G.
 *
 * predecessor refers to the vertices u such that there exists an edge (u, v) in graph G
 * takes as input the vertex, a pointer to the graph and a pointer to the array of vertices.
 * allocates and initializes the array of predecessors and returns the number of predecessors.
 */
size_t get_predecessors(size_t vertex, graph *G, size_t **predecessors) {
	// checks if the vertex is contained in the graph
	if(vertex >= G->n_verts || vertex < 0) {
		fprintf(stderr, "Error: vertex out of bounds\nvertex must be < n_vertex\n");
		return 0;
	}

	size_t row_index_start = G->csc_col_id[vertex];
	size_t row_index_end = G->csc_col_id[vertex + 1];
	size_t predecessor_count = row_index_end - row_index_start;

	// checks if there are even any predecessors
	if(predecessor_count > 0) {
		// allocates the required memory and handles for errors
		*predecessors = (size_t *) malloc(predecessor_count * sizeof(size_t));
		if(*predecessors == NULL) {
			fprintf(stderr, "Error allocating memory:\n%s\n", strerror(errno));
			return 0;
		}

		// then fills the array with the correct data
		for(size_t i = 0 ; i < predecessor_count ; ++i) {
			(*predecessors)[i] = G->csc_row_id[row_index_start + i];
		}
	}

	return predecessor_count;
}