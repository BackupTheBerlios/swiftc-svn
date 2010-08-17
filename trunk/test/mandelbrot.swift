# convert with:
# convert -depth 8 -size 6000x4000 gray:log mandelbrot.png

class Mandelbrot
    simd routine iterate (complex c) -> int num_iters
        real square = 0.0
        real max_square = 4.0

        int iter = 0
        int max_iter = 100

        complex z = 0.0, 0.0
        
        while (iter < max_iter) & (square < max_square)
            z = z.sq() + c
            square = z.abs_sq()

            iter = iter + 1
        end
        
        num_iters = iter
    end

    routine compute_mandelbrot()
        # constants
        index max_x = 6000x
        index max_y = 4000x
        vec2 max = max_x.to_real(), max_y.to_real()
        vec2 min = -2.0, -1.0
        vec2 pix_dist = 3.0/max_x.to_real(), 2.0/max_y.to_real()

        # result for one pixel row
        simd{int} result_data = max_x

        c_call start_timer()
        index pix_y = 0x
        while pix_y < max_y

            simd pix_x: 0x, max_x
                simd vec2 pix = (simd pix_x.to_real()) + simd_range{real}, simd pix_y.to_real()
                simd complex c = ((simd min) + pix * (simd pix_dist)).to_complex()
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
        c_call stop_timer()
    end

    routine main() -> int result
        ::compute_mandelbrot()
        result = 0
    end
end
