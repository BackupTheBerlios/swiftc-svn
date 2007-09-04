# Swift Language test file

class FOO
    int i
end

class Test
    Foo foo

    create (int peter, FOO fubert)
        int a
    end

    routine real re, int rs, inout int bs = test(FOO foo, int i)
        int a
        a = 5
    end

    routine real re, int rs, inout int bs = test(FOO foo)

        int a
        int b
        a = 5 * 6 + 7
        b = a
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
