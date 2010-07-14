simd class Complex
    real r
    real i

    simd create (real r, real i)
        .r = r
        .i = i
    end

    simd create (real r, real i)
        .r = r
        .i = i
    end

    simd reader + (Complex c) -> Complex result
        result.r = .r + c.r
        result.i = .i + c.i
    end

    simd reader - (Complex c) -> Complex result
        result.r = .r - c.r
        result.i = .i - c.i
    end

    simd reader * (Complex c) -> Complex result
        result.r = .r*c.r - .i*c.i
        result.i = .r*c.i + .i*c.r
    end

    simd reader / (Complex c) -> Complex result
        real den = c.r*c.r + c.i*c.i
        result.r = (.r*c.r + .i*c.i) / den
        result.i = (.i*c.r - .r*c.i) / den
    end

    simd writer sq() -> Complex result
        result.r = .r*.r - .i*.i
        result.i = 2.0 * .r*.i
    end

    reader print()
        c_call print_float(.r)
        c_call print_float(.i)
    end
end


class Mandelbrot
    routine iterate (Complex c) -> int num_iters
        #real square = 0.0
        #real max_square = 100.0

        int iter = 0
        int max_iter = 10

        Complex z = 0.0, 0.0
        
        while iter < max_iter
            z.sq()
            z = z + c
            iter = iter + 1
        end
        
        num_iters = iter
    end

    routine main() -> int result
        Complex c1 = 2.0, 5.0
        Complex c2 = 3.0, 7.0

        #(c1 * c2).print()
        (c1 / c2).print()

        result = 0
    end
end
