class Fibbonacci

    real x
    real64 y
    real64 z
    real w

    routine fibbonaci(int n) -> int result
        Fibbonacci fib
        Fibbonacci fib1
        Fibbonacci fib2

        if n == 0
            result = 0
            return
        end

        result = 1
        if n == 1
            return
        end

        int pre = 0
        while n >= 2
            int f = result
            result = result + pre
            pre = f

            n = n - 1
        end
    end
end
