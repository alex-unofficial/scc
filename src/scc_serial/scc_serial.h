#ifndef SCC_SERIAL_H
#define SCC_SERIAL_H

#include <graph.h>

// Implements the graph coloring algorithm to find the SCCs of G
ssize_t scc_coloring(const graph *G, vert_t **vertex_scc_id);

#endif
