#ifndef MATRIX_H
#define MARTIX_H

#include <stdio.h>

/* cs_pattern_matrix is a struct used to represent sparse pattern matrices, in the CSR or CSC formats.
 *
 * the CSR and CSC formats store only the nonzero elements of the sparse matrix.
 * in the case of CSR, row_id stores the row indices of the nonzero elements
 * while col_id stores the position in the row_id array where the nonzero elements
 * of the column begins. the opposite is true for CSC.
 *
 * since this is a pattern matrix, we dont store the value of nonzero elements, 
 * but only their position in the matrix.
 */
typedef struct {
	int is_csc;
	int is_csr;

	size_t n_rows;
	size_t n_cols;
	size_t n_nz;

	size_t *row_id;
	size_t *col_id;

} cs_pattern_matrix;

// Initialize a matrix struct in the CSC or CSR format.
size_t initialize_csc_matrix(cs_pattern_matrix *csc, size_t n_rows, size_t n_cols, size_t n_nz);
size_t initialize_csr_matrix(cs_pattern_matrix *csr, size_t n_rows, size_t n_cols, size_t n_nz);

// Free the memory allocated to a matrix struct
void free_matrix(cs_pattern_matrix *mtx);

// Imports a matrix from a MatrixMarket .mtx file as a CSC and CSR compressed matrix.
void import_matrix(char *mtx_fname, cs_pattern_matrix *csr, cs_pattern_matrix *csc);

#endif
