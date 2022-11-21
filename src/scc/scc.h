#ifndef SCC_H
#define SCC_H

#include <graph.h>

// Returns true if v is a trivial SCC
int is_trivial_scc(size_t v, const graph *G, const size_t *is_vertex);

// Implements the graph coloring algorithm to find the SCCs of G
size_t scc_coloring(const graph *G, size_t **vertex_scc_id);

#endif
