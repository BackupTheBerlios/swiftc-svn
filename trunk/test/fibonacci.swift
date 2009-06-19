class Fibbonacci

    routine fibbonaci(int n) -> int result
        if n == 0
            result = 0
            return
        end

        result = 1

        if n == 1
            return
        end

        int pre = 0
        int counter = n

        while counter >= 2
            int f = result
            result = result + pre
            pre = f

            counter = counter - 1
        end

    end

    routine main() -> int result
        int fib = ::fibbonaci(12)
        c_call print_int(fib)
        result = 0
    end

end
