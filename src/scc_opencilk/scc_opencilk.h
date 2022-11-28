#ifndef SCC_OPENCILK_H
#define SCC_OPENCILK_H

#include <graph.h>

// Implements the graph coloring algorithm to find the SCCs of G
ssize_t cilk_scc_coloring(const graph *G, vert_t **vertex_scc_id);

#endif
