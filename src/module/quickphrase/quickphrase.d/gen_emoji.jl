#!/usr/bin/julia -f

# Extract emoji's from Julia's REPL =)

fname = if length(ARGS) >= 1
    ARGS[1]
else
    "/dev/stdout"
end

open(fname, "w") do io
    dict = Base.REPLCompletions.emoji_symbols
    for key in sort(collect(keys(dict)))
        m = match(r"\\:(.*):", key)
        @assert m !== nothing
        println(io, ":$(m[1]) $(dict[key])")
    end
end
