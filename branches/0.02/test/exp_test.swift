class Test
    routine main() -> int result
        int i = -89.0 . bitcast_to_int()

        repeat
            real r = i.bitcast_to_real()

            real c_r = c_call real expf(r)
            real s_r = math::exp(r)

            if c_r.bitcast_to_int() != s_r.bitcast_to_int()
                c_call print_int(i)
                c_call print_float(r)
                c_call print_hexfloat(c_r)
                c_call print_hexfloat(s_r)
                c_call println()
            end

            i = i + 1
        until i.bitcast_to_real() == 89.0

        result = 0
    end
end
