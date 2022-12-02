/* scc - find number of sccs in a graph
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

#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>
#include <scc_serial.h>
#include <scc_pthreads.h>

int main(int argc, char **argv) {
	char* mtx_fname = NULL;
	if(argc < 2) {
		fprintf(stderr, "Error reading input arguments: %s\nUsage:\t%s mtx_file.mtx\n", 
				strerror(EINVAL), argv[0]);
		exit(EINVAL);
	}
	mtx_fname = argv[1];

	printf("=== importing graph ===\n");
	printf("file: %s\n", mtx_fname);
	graph *G = import_graph(mtx_fname);
	if(G == NULL) return -1;

	printf("number of vertices = %zu\n", G->n_verts);
	printf("number of edges = %zu\n", G->n_edges);

	printf("\n");

	struct timespec t1, t2;
	double elapsedtime;

	printf("=== serial SCC algorithm ===\n");
	vert_t *scc_id;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	ssize_t n_scc = scc_coloring(G, &scc_id);
	clock_gettime(CLOCK_MONOTONIC, &t2);

	if(n_scc == -1) {
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_nsec - t1.tv_nsec) / 10000000000.0;
	printf("total time: %0.6f sec\n", elapsedtime);

	printf("\n");

	printf("=== parallel SCC algorithm ===\n");
	vert_t *p_scc_id;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	ssize_t p_n_scc = p_scc_coloring(G, &p_scc_id);
	clock_gettime(CLOCK_MONOTONIC, &t2);

	if(p_n_scc == -1) {
		free_graph(G);
		return -1;
	}

	printf("number of SCCs = %zd\n", n_scc);

	elapsedtime = (t2.tv_sec - t1.tv_sec);
	elapsedtime += (t2.tv_nsec - t1.tv_nsec) / 10000000000.0;
	printf("total time: %0.6f sec\n", elapsedtime);

	printf("\n");

	printf("=== error checking ===\n");
	int num_errors = 0;

	if(n_scc != p_n_scc) {
		printf(
			"%3d: non matching number of SCCs -- %zd (serial) != %zd (parallel)\n", 
			num_errors++, n_scc, p_n_scc
		);
	}

	if(n_scc > G->n_verts) {
		printf(
			"%3d: invalid number of SCCs (serial) -- n_scc = %zd > n_verts = %zd\n", 
			num_errors++, n_scc, G->n_verts
		);
	}

	if(p_n_scc > G->n_verts) {
		printf(
			"%3d: invalid number of SCCs (parallel) -- n_scc = %zd > n_verts = %zd\n", 
			num_errors++, p_n_scc, G->n_verts
		);
	}

	for(size_t i = 0 ; i < G->n_verts ; i++) {
		if(scc_id[i] != p_scc_id[i]) {
			printf(
				"%3d: non matching scc id at index %zu -- %u (serial) != %u (parallel)\n",
				num_errors++, i, scc_id[i], p_scc_id[i]
			);
		}

		if(scc_id[i] > G->n_verts) {
			printf(
				"%3d: invalid scc id (serial) -- scc_id[%zu] = %u < n_verts = %zu\n",
				num_errors++, i, scc_id[i], G->n_verts
			);
		}

		if(p_scc_id[i] > G->n_verts) {
			printf(
				"%3d: invalid scc id (parallel) -- scc_id[%zu] = %u < n_verts = %zu\n",
				num_errors++, i, p_scc_id[i], G->n_verts
			);
		}
	}
	printf("errors found: %d", num_errors);
	
	printf("\n");

	free(scc_id);
	free(p_scc_id);

	free_graph(G);

	return 0;
}
