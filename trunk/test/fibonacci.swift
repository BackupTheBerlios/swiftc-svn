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

    # routine main()
    #     int result = ::fibbonaci(12)
    #     vc_call printf("fib(12) = %i", result)
    # end

end
