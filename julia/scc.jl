using Pkg
Pkg.add("MatrixMarket")
Pkg.add("DataStructures")

using SparseArrays
using MatrixMarket

using DataStructures


function N(v, G)
    p = sparsevec(zeros(G.m));
    p[v] = 1;

    q = G' * p;
    return q.nzind;
end


function P(v, G)
    p = sparsevec(zeros(G.m));
    p[v] = 1;

    q = G * p;
    return q.nzind;
end


function bfs(root, G, isactive, color, col_array)
    visited = fill(false, G.m);
    visited[root] = true;

    q = Queue{Int}();
    enqueue!(q, root);

    vertices = Vector{Int}();

    while !isempty(q)
        v = dequeue!(q);
        append!(vertices, v);

        front = N(v, G);

        for w in front
            if isactive[w] && !visited[w] && col_array[w] == color
                visited[w] = true;

                enqueue!(q, w);
            end
        end
    end

    return vertices;
end


function bfs(root, G, isactive)
    return bfs(root, G, isactive, 1, ones(G.m));
end


function bfs(root, G)
    return bfs(root, G, fill(true, G.m), 1, ones(G.m));
end


function istrivial(v, G)
    n = N(v, G);
    p = P(v, G);

    if length(n) == 0 || length(p) == 0
        return true;
    end

    if length(n) == 1 && n[1] == v
        return true;
    end

    if length(p) == 1 && p[1] == v
        return true;
    end

    return false;
end


function scccoloring(G)
    sccs = Dict{Int, AbstractVector}();
    
    isactive = fill(true, G.m);

    # trimming
    removed = true
    while removed
        println("loop 0")
        removed = false

        Threads.@threads for v in 1:G.m
            println("loop 0.1 - $v")
            if isactive[v] && istrivial(v, G)
                sccs[v] = [v];
                isactive[v] = false;

                removed = true;
            end
        end
    end


    # main loop
    while (true in isactive)
        println("loop 1")

        colors = Vector(1:G.m);

        changedcolor = true;
        while changedcolor
            println("loop 1.1")
            changedcolor = false;

            Threads.@threads for v in 1:G.m
                println("loop 1.1.1 - $v")
                if isactive[v]
                    for u in P(v, G)
                        println("loop 1.1.1.1 - $v $u")
                        if isactive[u] && colors[v] > colors[u]
                            colors[v] = colors[u];
                            changedcolor = true;
                        end
                    end
                end
            end

        end

        uniquecolors = Vector{Int}();
        for v in 1:G.m 
            println("loop 1.2 - $v")
            if isactive[v] && colors[v] == v
                append!(uniquecolors, v);
            end
        end

        Threads.@threads for c in uniquecolors
            println("loop 1.3 - $c")
            sccs[c] = bfs(c, sparse(G'), isactive, c, colors);
            isactive[sccs[c]] .= false;
        end
    end
    
    return sccs;
end


G = MatrixMarket.mmread(ARGS[1]);

println("nthreads = $(Threads.nthreads())");
@time sccs = scccoloring(G);
println("nsccs =  $(length(keys(sccs)))");
