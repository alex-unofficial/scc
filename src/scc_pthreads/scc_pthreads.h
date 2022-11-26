#ifndef SCC_PTHREADS_H
#define SCC_PTHREADS_H

#include <stdlib.h>

#include <graph.h>

// Implements the graph coloring algorithm to find the SCCs of G
ssize_t p_scc_coloring(const graph *G, vert_t **vertex_scc_id);

#endif
