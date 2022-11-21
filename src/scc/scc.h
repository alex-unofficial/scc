#ifndef SCC_H
#define SCC_H

#include <graph.h>

// Returns true if v is a trivial SCC
int is_trivial_scc(size_t v, graph *G);

// Implements the graph coloring algorithm to find the SCCs of G
size_t scc_coloring(graph *G, size_t **vertex_scc_id);

#endif
