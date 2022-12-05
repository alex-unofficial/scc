# scc
## find number of sccs in a graph

This program was built as an assignment for my Parallel & Distributed Comp. Systems
class in University.

It finds the number of SCCs in a graph given using a 
[MatrixMarket](https://math.nist.gov/MatrixMarket/formats.html) file, using one of
three parallel implementations using `POSIX threads`, `OpenMP` or `OpenCilk`.

Compiling
----------
In order to compile the program you need the following dependencies: 
[gcc](https://gcc.gnu.org/install/) and [OpenCilk's clang](https://www.opencilk.org/doc/users-guide/install/).
You also need `make` in order to run the Makefile

note:
if you built OpenCilk's clang from source, you need to install it to `$PATH`
before using it, or change the Makefile to point to the location of the clang binary.


To clone the repository run:
```bash
git clone https://github.com/alex-unofficial/scc
cd scc
```

There are three seperate implementations of the program, all living in seperate
git branches. `master` contains the `pthreads` implementation, and the `opencilk`
and `openmp` branches contain their respective implementations.

To build any specific implementation navigate to the branch that contains it and
run `make all`. for example to build the `openmp` implementation do
```
git checkout openmp
make all
```

Running `make all` as opposed to just `make` is important after each git checkout, 
since I have it set up such that it runs `make clean` before it builds the binaries. 

This is done because the compilers and libraries used for the different implementations
are incopatible with each other.


After building, the binary is in `./bin/scc`.


Running the program
-------------------
To run the program simply do
```bash
./bin/scc mtx_file.mtx
```
where mtx_file.mtx is a valid matrix file.

the program accepts some command line options
to display the help text run
```bash
./bin/scc -h
```

you can specify that you want to run just the serial or just the parallel
implementation with `-s` and `-p` respectively.
```bash
./bin/scc [-s|-p] mtx_file.mtx
```

additionally with the `-n` option you can specify the number of threads to use
```bash
./bin/scc [-n nthreads] mtx_file.mtx
```

the program by default will run both the serial and parallel implementations, measure the
time it takes to run the algorithm, then check for errors

Licence
-------
```
Copyright (C) 2022  Alexandros Athanasiadis

This file is part of scc

scc is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

scc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>. 
```
