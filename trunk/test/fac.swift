class Fac
    routine fac(real r) -> real result
        real n = 1.0
        result = 1.0
        while n < r
            n = n + 1.0
            result = result * n
        end
    end
end
