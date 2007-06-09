# swift language test file
class Test
    Foo foo;

    routine test()
        int a;
        int b;
        b = a;
        int c;
        c = -3;
        c = 5 + b;
        c = 5 * 7 + c * c + b;
        a = b*-c+b*-c;
        a = a +6;
    end

    writer hallo()
        real r;
        r = 7.0;

        bool b;

        b = r * 5.0 > 7.0;

        if (6 == 8)
            if (true)
                real r;
                r  = 6.0;

                if (r > 8.0)
                    r = 9.0;
                else
                    bool z ;
                    z = false;
                end
            else
                b = true;
            end

         else
            if (b)
                bool b;
                b  = false;
            end
        end

    real u;
    u = 5.0;

    end
end
