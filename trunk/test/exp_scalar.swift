class Math
    routine main() -> int result
        array{real} a = 40000000x
        index i = 0x
        while i < 40000000x
            a[i] = c_call real rand_float()
            i = i + 1x
        end

        c_call start_timer()
        i = 0x
        while i < 40000000x
            a[i] = math::exp(a[i])
            i = i + 1x
        end
        c_call stop_timer()

        result = 0
    end
end
