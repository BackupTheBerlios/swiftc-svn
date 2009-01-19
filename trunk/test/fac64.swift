class Fac
    routine fac(real64 r) -> real64 result
        real64 n = 1.0q
        result = 1.0q
        while n < r
            n = n + 1.0q
            result = result * n
        end
    end
end
