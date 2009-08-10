class Foo
    index x
    real r
end

class Test
    routine main() -> int result
        array{index} a = 20x
        index i = 0x

        # write
        while i < 20x
            a[i] = i
            i = i + 1x
        end

        i = 0x
        # load and print
        while i < 20x
            c_call print_int( a[i] )
            i = i + 1x
        end

        index sum = 0x
        i = 0x

        # sum up
        while i < 20x
            sum = sum + a[i]
            i = i + 1x
        end

        c_call print_int(sum)

        # array{Foo} af = 20x

        # i = 0x
        # # load and print
        # # write
        # while i < 20x
        #     af[i].x = i
        #     af[i].r = 5.0
        #     i = i + 1x
        # end

        # i = 0x
        # # load and print
        # while i < 20x
        #     c_call print_float( af[i].r )
        #     i = i + 1x
        # end

    end
end
