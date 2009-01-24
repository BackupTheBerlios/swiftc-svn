# Swift Language test file
class Test
    Foo foo

    routine test(int i, int j, real uu) -> real re, int rs, inout int bs
        int z
        z = 5 + 4 * j
        j = 7
    end

    create ()
        int afdafdsaf
        afdafdsaf = 5 + 7
    end
    create (real r)
        int z
        z = 5
        real64 foo = 1.0q
        real a = 1.0
        real b = 2.0
        real c = 3.0
        real d = a - b - 8.0 - 5.0 * c - a + 9.0
    end

    routine test(real foo) -> real re, int rs, inout int bs
        int e = 4

        real gg = 2.0 + 3.0

        int8 gh = 2b - 5b

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
        int aa = 1
        int bb = 2
        int cc = aa + bb
        int dd = cc + aa + bb
        bool zz = aa < bb

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

                while t > 3.1434
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
        real64 d = 5.0q
    end
end

