#ifndef SCC_H
#define SCC_H

#include <graph.h>

int is_trivial_scc(size_t v, graph *G);

size_t scc_coloring(graph *G, size_t **vertex_scc_id);

#endif
