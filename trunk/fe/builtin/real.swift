class real
    constructor (real r)
    end

    operator real + - * / (real r1, real r2)
    end

    operator += -= *= /= (real r1)
    end

    operator inc dec
    end

    operator bool < > <= >= == != (real r1, real r2)
    end

    iterator real each(real begin, real ending, real step = 1)
    end

    iterator real up(real begin, real ending, real step = 1)
    end

    iterator real down(real begin, real ending, real step = -1)
    end
end
