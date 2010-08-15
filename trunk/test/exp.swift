class Math

    simd routine small_fix(real x) -> real result
        result = x.to_int().to_real()
    end

    simd routine small_floor(real x) -> real result
        result = ::small_fix(x)

        if result > x
            result = result - 1.0
        end
    end

    simd routine small_ceil(real x) -> real result
        result = ::small_fix(x)

        if result < x
            result = result + 1.0
        end
    end

    simd routine small_round(real x) -> real result
        result = ::small_floor(x + 0.5)
    end

    simd routine exp(real arg) -> real result
        # make working copy
        real x = arg

        real exp_lo = -88.3762626647949 # less yields 0
        # clamp to this value
        if x < exp_lo
           x = exp_lo
        end

        # exp(x) = exp(g + n log 2) 
        #        = exp(g) * exp(n log 2)
        #        = exp(g) * exp(log 2^n)
        #        = exp(g) * 2^n

        # express exp(x) as exp(g + n*log(2))
        real         log_e_2 = 1060208640u.bitcast_to_real() #   ln(2)
        real one_div_log_e_2 = 1069066811u.bitcast_to_real() # 1/ln(2)
        real magic_const     = 3109978243u.bitcast_to_real() # -0.000212.., whatever this is?!
        real n = ::small_round(one_div_log_e_2 * x)
        real g = x - n * log_e_2 - n * magic_const

        # calc exp(g) via exp-series
        real f2 = (1.0q / (2.0q)).to_real()
        real f3 = (1.0q / (2.0q*3.0q)).to_real()
        real f4 = (1.0q / (2.0q*3.0q*4.0q)).to_real()
        real f5 = (1.0q / (2.0q*3.0q*4.0q*5.0q)).to_real()
        real f6 = (1.0q / (2.0q*3.0q*4.0q*5.0q*6.0q)).to_real()
        real f7 = (1.0q / (2.0q*3.0q*4.0q*5.0q*6.0q*7.0q)).to_real()

        real exp_g = 1.0+g*(1.0+g*(f2+g*(f3+g*(f4+g*(f5+g*(f6+g*f7))))))
        #c_call print_float( exp_g)

        # build 2^n
        #real pow2n = (((n.to_int() + 127) * 8388608) & 2139095040).bitcast_to_real()
        real pow2n = (((n.to_int() + 127) * 8388608) ).bitcast_to_real()
        #c_call print_float( n)
        #c_call print_float( pow2n)

        result = exp_g * pow2n
    end

    routine main() -> int result
        c_call print_float( ::exp( 88.0) )
        c_call print_float( ::exp(-88.0) )
        c_call print_float( ::exp( 89.0) )
        c_call print_float( ::exp(-89.0) )
        result = 0
    end
end
