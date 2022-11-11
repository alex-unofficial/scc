#include "matrix.h"

#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <mmio.h>

int initialize_csc_matrix(cs_pattern_matrix *csc, size_t n_rows, size_t n_cols, size_t n_nz) {
	csc->is_csc = 1;
	csc->is_csr = 0;

	csc->n_rows = n_rows;
	csc->n_cols = n_cols;
	csc->n_nz = n_nz;

	csc->row_id = (size_t *) malloc(n_nz * sizeof(size_t));
	csc->col_id = (size_t *) malloc((n_cols + 1) * sizeof(size_t));

	if(csc->row_id == NULL || csc->col_id == NULL) {
		return -1;
	}
}

int initialize_csr_matrix(cs_pattern_matrix *csr, size_t n_rows, size_t n_cols, size_t n_nz) {
	csr->is_csc = 0;
	csr->is_csr = 1;

	csr->n_rows = n_rows;
	csr->n_cols = n_cols;
	csr->n_nz = n_nz;

	csr->col_id = (size_t *) malloc(n_nz * sizeof(size_t));
	csr->row_id = (size_t *) malloc((n_rows + 1) * sizeof(size_t));

	if(csr->row_id == NULL || csr->col_id == NULL) {
		return -1;
	}
}

void free_matrix(cs_pattern_matrix *mtx) {
	free(mtx->row_id);
	free(mtx->col_id);
}

int comp_row(const void *a, const void *b) {
	size_t *arg_a = *(size_t**)a;
	size_t *arg_b = *(size_t**)b;

	if(arg_a[0] < arg_b[0]) return -1;
	if(arg_a[0] > arg_b[0]) return 1;
	return 0;
}

int comp_col(const void *a, const void *b) {
	size_t *arg_a = *(size_t**)a;
	size_t *arg_b = *(size_t**)b;
	
	if(arg_a[1] < arg_b[1]) return -1;
	if(arg_a[1] > arg_b[1]) return 1;
	return 0;
}

int import_matrix(char *mtx_fname, cs_pattern_matrix *csr, cs_pattern_matrix *csc) {
	FILE *mtx_file = NULL;
	mtx_file = fopen(mtx_fname, "r"); if(mtx_file == NULL) { int err = errno;
		fprintf(stderr, "Error opening file: %s\n%s\n", mtx_fname, strerror(err));
		exit(err);
	}

	int mm_read_err_code = 0;

	MM_typecode mtx_type;
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

	size_t n_rows = 0;
	size_t n_cols = 0;
	size_t n_nz = 0;

	if(mm_is_coordinate(mtx_type)) {
		mm_read_err_code = mm_read_mtx_crd_size(mtx_file, (int *) &n_rows, (int *) &n_cols, (int *) &n_nz);
	} else {
		fprintf(stderr, "Invalid matrix type");
		fclose(mtx_file);
		exit(1);
	}

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

	size_t** indices;
	indices = (size_t **) malloc(n_nz * sizeof(size_t *));
	for(size_t i = 0 ; i < n_nz ; ++i) {
		indices[i] = (size_t *) malloc(2 * sizeof(size_t));

		size_t row, col;

		if(mm_is_pattern(mtx_type)) {
			fscanf(mtx_file, "%d %d\n", &row, &col);
		} else if(mm_is_integer(mtx_type)) {
			size_t val;
			fscanf(mtx_file, "%d %d %d\n", &row, &col, &val);
		} else if(mm_is_real(mtx_type)) {
			double val;
			fscanf(mtx_file, "%d %d %f\n", &row, &col, &val);
		} else {
			fprintf(stderr, "Error reading from MatrixMarket file: %s\n", mtx_fname);
			fclose(mtx_file);
			exit(1);
		}

		indices[i][0] = row - 1;
		indices[i][1] = col - 1;
	}
	fclose(mtx_file);

	qsort(indices, n_nz, sizeof(size_t *), &comp_col);
	initialize_csc_matrix(csc, n_rows, n_cols, n_nz);

	csc->col_id[0] = 0;
	csc->col_id[n_cols] = n_nz;

	size_t nz_col_count = 0;
	size_t col_id = 1;

	size_t prev_col = indices[0][1];

	for(size_t i = 0 ; i < n_nz ; ++i) {
		size_t row = indices[i][0];
		size_t col = indices[i][1];

		csc->row_id[i] = row;

		if(col != prev_col) {
			csc->col_id[col_id] = csc->col_id[col_id - 1] + nz_col_count;

			col_id += 1;
			prev_col = col;
			nz_col_count = 0;
		}

		nz_col_count += 1;
	}

	qsort(indices, n_nz, sizeof(size_t *), &comp_row);
	initialize_csr_matrix(csr, n_rows, n_cols, n_nz);

	csr->row_id[0] = 0;
	csr->row_id[n_rows] = n_nz;

	size_t nz_row_count = 0;
	size_t row_id = 1;

	size_t prev_row = indices[0][0];

	for(size_t i = 0 ; i < n_nz ; ++i) {
		size_t row = indices[i][0];
		size_t col = indices[i][1];

		csr->col_id[i] = col;

		if(row != prev_row) {
			csr->row_id[row_id] = csr->row_id[row_id - 1] + nz_row_count;

			row_id += 1;
			prev_row = row;
			nz_row_count = 0;
		}

		nz_row_count += 1;
	}

	for(size_t i = 0 ; i < n_nz ; ++i) {
		free(indices[i]);
	}
	free(indices);

	return 0;
}
