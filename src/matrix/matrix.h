#ifndef MATRIX_H
#define MARTIX_H

#include <stdio.h>

typedef struct {
	int is_csc;
	int is_csr;

	size_t n_rows;
	size_t n_cols;
	size_t n_nz;

	size_t *row_id;
	size_t *col_id;

} cs_pattern_matrix;

int initialize_csc_matrix(cs_pattern_matrix *csc, size_t n_rows, size_t n_cols, size_t n_nz);
int initialize_csr_matrix(cs_pattern_matrix *csr, size_t n_rows, size_t n_cols, size_t n_nz);

void free_matrix(cs_pattern_matrix *mtx);

int import_matrix(char *mtx_fname, cs_pattern_matrix *csr, cs_pattern_matrix *csc);

#endif
