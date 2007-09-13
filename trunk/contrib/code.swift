# Swift Language test file

class Test
    Foo foo

    create ()
        int afdafdsaf
        afdafdsaf = 5 +7
    end
    create (real r)
        int z
        z = 5
    end

    routine real re, int rs, inout int bs = test(FOO foo, int i)
        int z
        z = 5
    end

    routine real re, int rs, inout int bs = test(FOO foo)

        int e = 4, 7.0, 6
        int b = 8
        a = 5 * 6 + 7
        b = 5, 7.0, 8 + b * 7, 9, 10, 8, 5, 7, 8
        int c
        c = -3
        c = 5 + b
        c = 5 * 7 + c * c + b
        a = b*-c+b*-c
        a = a +6
    end

    writer hallo()
        real r
        r = 7.0

        bool b

        b = r * 5.0 > 7.0

        if 6 == 8
            if true
                real r
                r  = 6.0

                if r > 8.0
                    r = 9.0
                else
                    bool z
                    z = false
                end
            else
                b = true
            end

         else
            real t
            t = 1.2

            if b
                bool b
                b  = false
                t = 7.0
            else
                if b
                    real u
                    u = 5.0 * r
                end
                t = 8.0
            end

            t = t * 0.3
        end

    real u
    u = 5.0

    end
end

class FOO
    int i
end

