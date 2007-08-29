class Test
    routine int i, uint ui, inout real r1 = test(real r)
        i = r * 6.0
        ui = r * 8.0
        r1 += 5.0
    end

    routine int main(array{string} args)
        if args < 4
            printf("less then 2 args given")
            return 0
        end

        int i = args[1].to()
        uint ui = args[2].to()
        real r1 = args[3].to()
        real r2 = args[4].to()
        i, ui, r1 = test(r2)

        real r1 = args[3].to()
        i, ui, r1 = test(r2)

        return 0
    end
end

simd{Vec3} a
simd{Vec3} b
simd{Vec3} c

c = cross(a, b)


routine test()
    array{real, 40} a

    for real r in a
        r = 5
    end

    # each

    for index i = a.each() # means every element, no particular order
        a[i] = i

    # up

    for index i in up(3, 9) # means 3, 4, 5, 6, 7, 8
        a[i] = i

    for index i in up(3, 9, 2) # means 3, 5, 7
        a[i] = i

    for index i in up(9, 3, -2) # means nothing
        a[i] = i

    # down

    for index i in down(3, 9) # means 8, 7, 6, 5, 4, 3
        a[i] = i

    for index i in down(3, 9, -2) # means 7, 5, 3
        a[i] = i

    for index i in down(3, 9, 2) # means nothing
        a[i] = i

    simd{Vec3} a
    simd{Vec3} b
    simd{Vec3} c

    a = cross(b, c)
    a[3..5] = b[4..6] + c[5..7]

    real r
    int i = 7

    Map{Test} m

    [Map{Test}.Node n, bool b] = m.find(t)

    r, inout i = fooBar()

    for auto key, auto value in map.each()
        println(key, value)

    routine Key key, Value value each(index i)


    Bla foo(Blub i)
end


class Test
{
    void member();

    void constMember() const;

    static staticMember();


    reader int i1 member()
        i1 = 5

        return
    end

    reader int member()
        return 5
    end

    reader int summe = knue(int summand1 , int summand2)
    end

    reader int i1, int i2 = member(real r, string str)
        i1 = 5
        i2 = 6

        return
    end

    writer constMember()
    routine staticMember()

    constructor (int i, real r)
        # ....
    end
};

routine test()
    Test test = 5, 6.0

    Map m = {"Bier", "Penis", "Fotze"}

    Map^ m = new Map("Bier", "Penis", "Fotze")

    Map m("Bier", "Penis", "Fotze");

    string str("fjdkjdfk");

    str = "fjkfdjk";
end
