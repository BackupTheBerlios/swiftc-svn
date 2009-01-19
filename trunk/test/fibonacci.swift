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
        while n >= 2
            int temp = result
            result = result + pre
            pre = temp

            n = n - 1
        end
    end
end
