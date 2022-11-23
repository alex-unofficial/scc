#ifndef SCC_PTHREADS
#define SCC_PTHREADS

#include <stdlib.h>

#include <graph.h>

// Implements the graph coloring algorithm to find the SCCs of G
size_t p_scc_coloring(const graph *G, size_t **vertex_scc_id);

#endif
