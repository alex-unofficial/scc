#include "matrix.h"

#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <mmio.h>

/* Initialize a matrix struct in the Compressed Sparse Column format.
 *
 * takes as input a pointer to the struct to be initialized and the number of rows, columns and 
 * non zero elements, then allocates the required memory to row_id and col_id.
 *
 * the matrix struct should be freed by using free_matrix(&csc)
 */
size_t initialize_csc_matrix(cs_pattern_matrix *csc, size_t n_rows, size_t n_cols, size_t n_nz) {
	// initializing the variables
	csc->is_csc = 1;
	csc->is_csr = 0;

	csc->n_rows = n_rows;
	csc->n_cols = n_cols;
	csc->n_nz = n_nz;

	// in CSC format, row_id is of size n_nz and col_id of size n_cols+1
	csc->row_id = (size_t *) malloc(n_nz * sizeof(size_t));
	csc->col_id = (size_t *) malloc((n_cols + 1) * sizeof(size_t));

	if(csc->row_id == NULL || csc->col_id == NULL) {
		return errno;
	}

	return 0;
}

/* Initialize a matrix struct in the Compressed Sparse Row format.
 *
 * takes as input a pointer to the struct to be initialized and the number of rows, columns and 
 * non zero elements, then allocates the required memory to row_id and col_id.
 *
 * the matrix struct should be freed by using free_matrix(&csc)
 */
size_t initialize_csr_matrix(cs_pattern_matrix *csr, size_t n_rows, size_t n_cols, size_t n_nz) {
	// initializing the variables
	csr->is_csc = 0;
	csr->is_csr = 1;

	csr->n_rows = n_rows;
	csr->n_cols = n_cols;
	csr->n_nz = n_nz;

	// in CSR format, col_id is of size n_nz and row_id of size n_cols+1
	csr->col_id = (size_t *) malloc(n_nz * sizeof(size_t));
	csr->row_id = (size_t *) malloc((n_rows + 1) * sizeof(size_t));

	if(csr->row_id == NULL || csr->col_id == NULL) {
		return errno;
	}

	return 0;
}

/* Free the memory allocated to a matrix struct
 *
 * takes as input a pointer to the struct and frees the memory allocated to row_id and col_id.
 */
void free_matrix(cs_pattern_matrix *mtx) {
	free(mtx->row_id);
	free(mtx->col_id);
}

/* Compare the first index given 2 pairs of integer points
 *
 * helper function to be used inside qsort.
 * takes as input a pair a,b of void* that point to pairs of numbers,
 * used for row and column indexes, and compares them based on their first element
 * effectively comparing between their row indices.
 *
 * returns -1 if row_a < row_b
 * 			1 if row_a > row_b
 * 			0 if row_a = row_b
 */
int comp_row(const void *a, const void *b) {
	size_t row_a = (*(size_t**)a)[0];
	size_t row_b = (*(size_t**)b)[0];

	if(row_a < row_b) return -1;
	if(row_a > row_b) return 1;
	return 0;
}

/* Compare the second index given 2 pairs of integer points
 *
 * helper function to be used inside qsort.
 * takes as input a pair a,b of void* that point to pairs of numbers,
 * used for row and column indexes, and compares them based on their second element
 * effectively comparing between their column indices.
 *
 * returns -1 if col_a < col_b
 * 			1 if col_a > col_b
 * 			0 if col_a = col_b
 */
int comp_col(const void *a, const void *b) {
	size_t col_a = (*(size_t**)a)[1];
	size_t col_b = (*(size_t**)b)[1];
	
	if(col_a < col_b) return -1;
	if(col_a > col_b) return 1;
	return 0;
}

/* Imports a matrix from a MatrixMarket .mtx file as a CSC and CSR compressed matrix.
 *
 * Takes as input the path to the .mtx file to be imported and pointers to the
 * matrix CSC and CSR structs.
 *
 * It initializes the structs based on the size of the matrix and fills their respective
 * arrays with the correct values.
 *
 * The .mtx file must be in the format sparse (coordinate) and general, and the values must
 * be either integer, pattern or real.
 *
 * we are concerned mostly with the shape of the graph the matrix represents so the
 * values are discarded, and only the location of the nonzero elements is saved.
 */
void import_matrix(char *mtx_fname, cs_pattern_matrix *csr, cs_pattern_matrix *csc) {

	// Attempt to open the file mtx_fname and checking for errors.
	FILE *mtx_file = NULL;
	mtx_file = fopen(mtx_fname, "r"); if(mtx_file == NULL) { 
		size_t err = errno;
		fprintf(stderr, "Error opening file: %s\n%s\n", mtx_fname, strerror(err));
		exit(err);
	}

	// The typecode struct stores information about the type of matrix the .mtx file represents
	MM_typecode mtx_type;

	// Attempt to read the banner of the matrix, handle errors
	size_t mm_read_err_code = 0;
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
		exit(1);
	}

	// the dimensions of the matrix and the nonzero elements
	size_t n_rows = 0;
	size_t n_cols = 0;
	size_t n_nz = 0;

	// Attempt to read size information, only if matrix is of type coordinate
	if(mm_is_coordinate(mtx_type)) {
		mm_read_err_code = mm_read_mtx_crd_size(mtx_file, (int *) &n_rows, (int *) &n_cols, (int *) &n_nz);
	} else {
		fprintf(stderr, "Invalid matrix type");
		fclose(mtx_file);
		exit(1);
	}

	// Handle errors related to reading the size information
	if(mm_read_err_code) {
		fprintf(stderr, "Error reading MatrixMarket matrix size: %s\n", mtx_fname);

		switch(mm_read_err_code) {
			case MM_PREMATURE_EOF:
				fprintf(stderr, "EOF encountered while reading matrix size\n\t");
				break;
			default:
				fprintf(stderr, "Unknown error code: %d\n", mm_read_err_code);
				break;
		}

		fclose(mtx_file);
		exit(1);
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
			fscanf_match_count = fscanf(mtx_file, "%d %d\n", &row, &col);
		} else if(mm_is_integer(mtx_type)) {
			size_t val;
			fscanf_match_count = fscanf(mtx_file, "%d %d %d\n", &row, &col, &val);
		} else if(mm_is_real(mtx_type)) {
			double val;
			fscanf_match_count = fscanf(mtx_file, "%d %d %f\n", &row, &col, &val);
		} else {
			fprintf(stderr, "MatrixMarket file is of unsupported format: %s\n", mtx_fname);
			fclose(mtx_file);
			exit(1);
		}

		if(fscanf_match_count == EOF) {
			size_t fscanf_err_code = -1;

			if(ferror(mtx_file)) {
				fscanf_err_code = errno;
				fprintf(stderr, "Error reading from %s:\n%s\n", mtx_fname, strerror(fscanf_err_code));
			} else {
				fprintf(stderr, "Error reading from %s:\nfscanf matching failure\n", mtx_fname);
			}
			
			fclose(mtx_file);
			exit(fscanf_err_code);
		} else if(fscanf_match_count == 0) {
			fprintf(stderr, "Error reading from %s:\nfscanf early matching failure\n", mtx_fname);

			fclose(mtx_file);
			exit(-1);
		}

		indices[i][0] = row - 1;
		indices[i][1] = col - 1;
	}
	// since we have read all the data from the .mtx file we can close it.
	fclose(mtx_file);

	// sort the data based on the column index in order to create the CSC format
	qsort(indices, n_nz, sizeof(size_t *), &comp_col);
	
	size_t mtx_init_err_code = 0;
	// initialize the CSC struct and handle errors
	mtx_init_err_code = initialize_csc_matrix(csc, n_rows, n_cols, n_nz);
	if(mtx_init_err_code) {
		fprintf(stderr, "Error initializing CSC matrix: %s\n%s\n", mtx_fname, strerror(mtx_init_err_code));
		exit(mtx_init_err_code);
	}

	// col_id[0] is always 0 and col_id[n_cols] is always n_nz
	csc->col_id[0] = 0;
	csc->col_id[n_cols] = n_nz;

	// nz_col_count refers to the number of entries that have the same column in a row
	// this will be used to populate col_id
	size_t nz_col_count = 0;

	// col_id_ptr refers to the position in the col_id array that the next entry must be written.
	size_t col_id_ptr = 1;

	// prev col holds the column index of the previous entry.
	size_t prev_col = indices[0][1];

	for(size_t i = 0 ; i < n_nz ; ++i) {
		size_t row = indices[i][0];
		size_t col = indices[i][1];

		// store the row in row_id as is
		csc->row_id[i] = row;

		// then check if the column has changed.
		if(col != prev_col) {
			// if it has, write to the proper col_id position, the number of
			// entries that have the same column index and add it to the previous value
			// in order to show where the next column index starts in the row_id array
			csc->col_id[col_id_ptr] = csc->col_id[col_id_ptr - 1] + nz_col_count;

			// increase the col_id position to be written, change prev_col to the current column index
			// and set the counter back to zero.
			col_id_ptr += 1;
			prev_col = col;
			nz_col_count = 0;
		}

		// then update the column counter
		nz_col_count += 1;
	}

	// then sort the entries based on row index, for the CSR format
	qsort(indices, n_nz, sizeof(size_t *), &comp_row);

	// initialize the CSR struct and handle errors
	mtx_init_err_code = initialize_csr_matrix(csr, n_rows, n_cols, n_nz);
	if(mtx_init_err_code) {
		fprintf(stderr, "Error initializing CSR matrix: %s\n%s\n", mtx_fname, strerror(mtx_init_err_code));
		exit(mtx_init_err_code);
	}

	// then initialize the CSR struct in the same way as above, 
	// with col_id and row_id switched.
	csr->row_id[0] = 0;
	csr->row_id[n_rows] = n_nz;

	size_t nz_row_count = 0;
	size_t row_id_ptr = 1;

	size_t prev_row = indices[0][0];

	for(size_t i = 0 ; i < n_nz ; ++i) {
		size_t row = indices[i][0];
		size_t col = indices[i][1];

		csr->col_id[i] = col;

		if(row != prev_row) {
			csr->row_id[row_id_ptr] = csr->row_id[row_id_ptr - 1] + nz_row_count;

			row_id_ptr += 1;
			prev_row = row;
			nz_row_count = 0;
		}

		nz_row_count += 1;
	}

	// finally free the memory in the indices array, since it is no longer needed.
	for(size_t i = 0 ; i < n_nz ; ++i) {
		free(indices[i]);
	}
	free(indices);
}
