/* OpenMP scc implementation header
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

#ifndef SCC_OPENMP_H
#define SCC_OPENMP_H

#include <graph.h>

// Implements the graph coloring algorithm to find the SCCs of G
ssize_t omp_scc_coloring(const graph *G, vert_t **vertex_scc_id);

#endif
