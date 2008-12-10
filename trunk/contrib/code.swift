# Swift Language test file

class Test
    Foo foo

    routine test(Foo foo, int i) -> real re, int rs, inout int bs
        int z
        z = 5 + 4
    end

    operator + (Test f1, Foo f2) -> Foo res
    end

    create ()
        int afdafdsaf
        afdafdsaf = 5 + 7
    end
    create (real r)
        int z
        z = 5
    end

    routine test(Foo foo) -> real re, int rs, inout int bs
        int e = 4

        if e > 5
            e = e + 2
        else
            e = e - 3
        end

        e = e * 2
    end

    writer hallo()
        real r
        r = 7.0
        bool b

        b = r * 5.0 > 7.0
        real foo
        foo = r + 2.0

        while b
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
                t = 5.0

                while t > 3.0
                    if b
                        bool b
                        b  = false
                        int bar
                        bar = 5 + 8
                        t = 7.0 + r
                        foo = 2.0
                    else
                        real bar = foo
                        if b
                            real u
                            u = 5.0 * r
                        bar = 4.0
                        end
                        t = 8.0
                    end

                    t = t * 0.3
                end
            end
        end

        real u
        u = 5.0

        while u < 8.0
            u = u + 1.0
        end
    end
end

class Foo
    create ()
        int i = 6
    end
end

