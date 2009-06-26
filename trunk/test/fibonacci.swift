class Fibonacci

    # iterative implementation
    routine fibonacci(int n) -> int result
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

    # recursive implementation
    routine fibonacci_rec(int n) -> int result
        if n == 0
            result = 0
            return
        end

        if n == 1
            result = 1
            return
        end
        
        result = ::fibonacci_rec(n - 1) + ::fibonacci_rec(n - 2)
    end

    # print fibonacci(0) till fibonacci(12) with the iterative and the recursive implementation
    routine main() -> int result
        int n = 0

        # for n = 0 .. 12
        while n < 13
            int fib = ::fibonacci(n)
            c_call print_int(fib)
            fib = ::fibonacci_rec(n)
            c_call print_int(fib)

            n = n + 1
        end

        result = 0
    end

end
