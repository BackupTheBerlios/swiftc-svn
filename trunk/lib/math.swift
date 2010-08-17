class math
    simd routine sfix(real x) -> real result
        result = x.to_int().to_real()
    end

    simd routine sfloor(real x) -> real result
        result = ::sfix(x)

        if result > x
            result = result - 1.0
        end
    end

    simd routine sceil(real x) -> real result
        result = ::sfix(x)

        if result < x
            result = result + 1.0
        end
    end

    simd routine sround(real x) -> real result
        result = ::sfloor(x + 0.5)
    end

    simd routine _k_cos(real x, real z) -> real y
        real coscof0 =  2.443315711809948E-005
        real coscof1 = -1.388731625493765E-003
        real coscof2 =  4.166664568298827E-002

        y = (((coscof0 * z + coscof1) * z + coscof2) * z - 0.5) * z + 1.0
    end

    simd routine _k_sin(real x, real z) -> real y
        real sincof0 = -1.9515295891E-4
        real sincof1 =  8.3321608736E-3
        real sincof2 = -1.6666654611E-1

        y = (((sincof0 * z + sincof1) * z + sincof2) * z * x) + x
    end

    simd routine _adjust_sign(int sign, real y) -> real result
        result = ((sign << 31) ^ y.bitcast_to_int()).bitcast_to_real()
    end

    simd routine _pre_sin_cos(real arg) -> real x, uint j
        real FOPI = 1.27323954473516
        real PIO4F = 0.7853981633974483096
        real DP1 = 0.78515625
        real DP2 = 2.4187564849853515625e-4
        real DP3 = 3.77489497744594108e-8
        real lossth = 8192.0
        real T24M1 = 16777215.0

        x = (arg.bitcast_to_int() & 0x7FFFFFFF).bitcast_to_real() # abs(arg)

        #if x > T24M1
        #    mtherr( "sinf", TLOSS )
        #    return(0.0)
        #end

        j = (FOPI * x).to_uint() # integer part of x/(PI/4)
        real y = j.to_real()

        # map zeros to origin
        if (j & 1u) > 0u
            j = j + 1u
            y = y + 1.0
        end

        if x > lossth
            #mtherr( "sinf", PLOSS )
            x = x - y * PIO4F
        else
            # Extended precision modular arithmetic
            x = ((x - y * DP1) - y * DP2) - y * DP3
        end
    end

    simd routine _sin_cos_helper(uint in_j, int in_sign) -> uint j, int sign
        j = in_j & 7u; # octant modulo 360 degrees

        # reflect in x axis
        if j > 3u
            sign = ~in_sign
            j = j - 4u
        else
            sign = in_sign
        end
    end

    simd routine cos(real arg) -> real result
        real x, uint j = ::_pre_sin_cos(arg)
        j, int sign = ::_sin_cos_helper(j, 0)
        real z = x * x

        real y

        if j > 1u
            sign = ~sign
        end

        if (j==1u) | (j==2u)
            y = ::_k_sin(x, z)
        else
            y = ::_k_cos(x, z)
        end

        result = ::_adjust_sign(sign, y)
    end

    simd routine sin(real arg) -> real result
        real x, uint j = ::_pre_sin_cos(arg)
        j, int sign = ::_sin_cos_helper(j, arg.bitcast_to_int() >> 31)
        real z = x * x
        real y

        if (j==1u) | (j==2u)
            y = ::_k_cos(x, z)
        else
            y = ::_k_sin(x, z)
        end

        result = ::_adjust_sign(sign, y)
    end

    simd routine sincos(real arg) -> real r_sin, real r_cos
        real x, uint j_sin = ::_pre_sin_cos(arg)
        uint j_cos = j_sin
        real z = x * x

        j_sin, int sign_sin = ::_sin_cos_helper(j_sin, arg.bitcast_to_int() >> 31)
        j_cos, int sign_cos = ::_sin_cos_helper(j_cos, 0)

        if j_cos > 1u
            sign_cos = ~sign_cos
        end

        real y_k_cos = ::_k_cos(x, z)
        real y_k_sin = ::_k_sin(x, z)

        real y_sin
        real y_cos

        if (j_sin==1u) | (j_sin==2u)
            y_sin = y_k_cos
        else
            y_sin = y_k_sin
        end

        if (j_cos==1u) | (j_cos==2u)
            y_cos = y_k_sin
        else
            y_cos = y_k_cos
        end

        r_sin = ::_adjust_sign(sign_sin, y_sin)
        r_cos = ::_adjust_sign(sign_cos, y_cos)
    end

    simd routine exp(real arg) -> real result
        # make working copy
        real x = arg

        # clamp to bounds
        real exp_lo = -88.3762626647949 # less yields 0
        real exp_hi =  88.3762626647949 # less yields inf
        if x < exp_lo
           x = exp_lo
        end
        if x > exp_hi
           x = exp_hi
        end

        # exp(x) = exp(g + n log 2) 
        #        = exp(g) * exp(n log 2)
        #        = exp(g) * exp(log 2^n)
        #        = exp(g) * 2^n

        # express exp(x) as exp(g + n*log(2))
        real         log_e_2 = 1060208640u.bitcast_to_real() #   ln(2)
        real one_div_log_e_2 = 1069066811u.bitcast_to_real() # 1/ln(2)
        real magic_const     = 3109978243u.bitcast_to_real() # -0.000212.., whatever this is?!
        real n = ::sround(one_div_log_e_2 * x)
        real g = x - n * log_e_2 - n * magic_const

        # calc exp(g) via exp-series
        real f2 = (1.0q / (2.0q)).to_real()
        real f3 = (1.0q / (2.0q*3.0q)).to_real()
        real f4 = (1.0q / (2.0q*3.0q*4.0q)).to_real()
        real f5 = (1.0q / (2.0q*3.0q*4.0q*5.0q)).to_real()
        real f6 = (1.0q / (2.0q*3.0q*4.0q*5.0q*6.0q)).to_real()
        real f7 = (1.0q / (2.0q*3.0q*4.0q*5.0q*6.0q*7.0q)).to_real()

        real exp_g = 1.0+g*(1.0+g*(f2+g*(f3+g*(f4+g*(f5+g*(f6+g*f7))))))

        # build 2^n
        real pow2n = ((n.to_int() + 127) << 23).bitcast_to_real()

        result = exp_g * pow2n
    end
end
