class Test
    simd routine bla(real r) -> real result
        result = r
    end

    routine main() -> int result
        simd{real} a = 40000000x
        index i = 0x
        while i < 40000000x
            a[i] = c_call real rand_float()
            i = i + 1x
        end

        c_call start_timer()
        simd i: 0x, 40000000x
            a@ = math::exp(a@)
        end
        c_call stop_timer()

        result = 0
    end
end
