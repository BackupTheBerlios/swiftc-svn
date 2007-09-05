class int
    create (int i)
    end

    operator int + - * / (int i1, int i2)
    end

    operator += -= *= /= (int i1)
    end

    operator inc dec
    end

    operator bool < > <= >= == != (int i1, int i2)
    end

    iterator int each(int begin, int ending, int step = 1)
    end

    iterator int up(int begin, int ending, int step = 1)
    end

    iterator int down(int begin, int ending, int step = -1)
    end
end
