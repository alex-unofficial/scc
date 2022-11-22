#ifndef SCC_H
#define SCC_H

#include <graph.h>

// Implements the graph coloring algorithm to find the SCCs of G
size_t scc_coloring(const graph *G, size_t **vertex_scc_id);

#endif
