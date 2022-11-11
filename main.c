#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <matrix.h>

int main(size_t argc, char **argv) {
	char* mtx_fname = NULL;
	if(argc < 2) {
		fprintf(stderr, "Error reading input arguments: %s\nUsage:\t%s mtx_file.mtx\n", 
				strerror(EINVAL), argv[0]);
		exit(EINVAL);
	}
	mtx_fname = argv[1];

	cs_pattern_matrix csc;
	cs_pattern_matrix csr;

	import_matrix(mtx_fname, &csr, &csc);

	printf("rows=%d cols=%d nnz=%d\n", csr.n_rows, csr.n_cols, csr.n_nz);
	printf("rows=%d cols=%d nnz=%d\n", csc.n_rows, csc.n_cols, csc.n_nz);

	free_matrix(&csc);
	free_matrix(&csr);

	return 0;
}
