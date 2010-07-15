# convert with:
# convert -depth 8 -size 6000x4000 gray:log mandelbrot.png

simd class Vec2
    real x
    real y

    simd create (real x, real y)
        .x = x
        .y = y
    end

    simd reader + (Vec2 v) -> Vec2 result
        result.x = .x + v.x
        result.y = .y + v.y
    end

    simd reader * (Vec2 v) -> Vec2 result
        result.x = .x * v.x
        result.y = .y * v.y
    end

    simd reader toComplex() -> Complex result
        result.r = .x
        result.i = .y
    end
end

simd class Complex
    real r
    real i

    simd create (real r, real i)
        .r = r
        .i = i
    end

    simd reader + (Complex c) -> Complex result
        result.r = .r + c.r
        result.i = .i + c.i
    end

    simd reader sq() -> Complex result
        result.r = .r*.r - .i*.i
        result.i = 2.0 * .r*.i
    end

    simd reader abs_sq() -> real result
        result = .r*.r + .i*.i
    end

    simd reader toVec2() -> Vec2 result
        result.x = .r
        result.y = .i
    end
end

class Mandelbrot
    simd routine iterate (Complex c) -> int num_iters
        real square = 0.0
        real max_square = 4.0

        int iter = 0
        int max_iter = 100

        Complex z = 0.0, 0.0
        
        while iter < max_iter 
            z = z.sq() + c

            square = z.abs_sq()
            if square >= max_square
                break
            end

            iter = iter + 1
        end
        
        num_iters = iter
    end

    routine compute_mandelbrot()
        # constants
        index max_x = 6000x
        index max_y = 4000x
        Vec2 max = max_x.to_real(), max_y.to_real()
        Vec2 min = -2.0, -1.0
        Vec2 pix_dist = 3.0/max_x.to_real(), 2.0/max_y.to_real()

        # result for one pixel row
        simd{int} result_data = max_x

        index pix_y = 0x
        while pix_y < max_y

            simd pix_x: 0x, max_x
                simd Vec2 pix = (simd pix_x.to_real()) + simd_range{real}, simd pix_y.to_real()
                simd Complex c = ((simd min) + pix * (simd pix_dist)).toComplex()
                result_data@ = ::iterate(c)
            end

            # print result
            #index tmp = 0x
            #while tmp < max_x
                #c_call print_byte(result_data[tmp])
                #tmp = tmp + 1x
            #end

            pix_y = pix_y + 1x
        end
    end

    routine main() -> int result
        ::compute_mandelbrot()
        result = 0
    end
end
