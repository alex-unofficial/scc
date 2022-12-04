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
#include <stdbool.h>

#include <unistd.h>
#include <ctype.h>

#include <time.h>

#include <errno.h>
#include <string.h>

#include <graph.h>
#include <scc_serial.h>
#include <scc_pthreads.h>

int main(int argc, char **argv) {

	const char *program_name = argv[0];

	bool run_serial = false;
	bool run_parallel = false;

	int opt;
	while((opt = getopt(argc, argv, "sp")) != -1) {
		switch(opt) {
		case 's':
			run_serial = true;
			break;
		case 'p':
			run_parallel = true;
			break;
		case '?':
			if(isprint(optopt))
				fprintf(stderr, "Error: unknown command-line option '-%c'\n", optopt);
			else
				fprintf(stderr, "Error: unknown option character '\\x%x'\n", optopt);

			exit(EINVAL);
		default:
			abort();
		}
	}

	if(!(run_serial || run_parallel)) {
		run_serial = true;
		run_parallel = true;
	}

	char* mtx_fname = NULL;
	if(optind >= argc) {
		fprintf(stderr, "Error reading input arguments: %s\nUsage:\t%s [-sp] mtx_file.mtx\n", 
				strerror(EINVAL), program_name);
		exit(EINVAL);
	}
	mtx_fname = argv[optind];

	printf("=== importing graph ===\n");
	printf("file: %s\n", mtx_fname);
	graph *G = import_graph(mtx_fname);
	if(G == NULL) return -1;

	printf("number of vertices = %zu\n", G->n_verts);
	printf("number of edges = %zu\n", G->n_edges);

	printf("\n");

	struct timespec t1, t2;
	double elapsedtime;

	ssize_t n_scc;
	vert_t *scc_id;
	if(run_serial) {
		printf("=== serial SCC algorithm ===\n");
		clock_gettime(CLOCK_MONOTONIC, &t1);
		n_scc = scc_coloring(G, &scc_id);
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
	}

	ssize_t p_n_scc;
	vert_t *p_scc_id;
	if(run_parallel) {
		printf("=== parallel SCC algorithm ===\n");
		clock_gettime(CLOCK_MONOTONIC, &t1);
		p_n_scc = p_scc_coloring(G, &p_scc_id);
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
	}

	if(run_serial && run_parallel) {
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
	}

	if(run_serial)
		free(scc_id);

	if(run_parallel)
		free(p_scc_id);

	free_graph(G);

	return 0;
}
