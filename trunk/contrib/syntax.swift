class Test
    int foo

    writer test(int i, uint ui) -> inout real r1
        i = r * 6.0
        ui = r * 8.0
        r1 += 5.0
        
        .foo = i
    end

    routine foo(inout int i, real r) -> int i
    end

    routine foobar()
        int i = 8

        int j = foo(inout i, r)
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

        a, b = t.setValue(r)

        bool b = t:getB()

        a:toArray()
        a:toSimd()

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

    reader knue(int summand1 , int summand2) -> int summe
    end

    reader int i1, int i2 = member(real r, string str)
        i1 = 5
        i2 = 6

        return
    end

    operator + (Complex c1, Complex c2) -> Complex result
        result.r = c1.r + c2.r
        result.i = c1.i + c2.i
    end

    operator = (Complex c) -> Complex result
        result.r = c.r
        result.i = c.i
    end


    writer constMember()
    routine staticMember()

    create (int i, real r)
        # ....
    end
end

routine test()
    Test test = 5, 6.0

    {}

    int i   = 4
    int8 i  = 4b
    int16 i = 4w
    int32 i = 4d
    int64 i = 4q

    uint i   = 4u
    uint8 i  = 4ub
    uint16 i = 4uw
    uint32 i = 4ud
    uint64 i = 4uq

    index i = 4x

    real r = 3.0
    real32 r = 3.0d
    real64 r = 3.0q

    int i = 434324234

    cplx c = {4, 5}
    c += {3, 8}

    i = 5
    i = {5}
    i = {{5}}

end

class cplx
    real r_
    real i_

    create (real r, real i)
        r_ = r
        i_ = i
    end
end

class vec2
    real x_
    real y_

    create (real x, real y)
        x_ = x
        y_ = y
    end
end

class Foo
    create (cplx c)
    end

    create (vec2 v)
    end
end

Foo f = { vec2(5.0, 6.0) }

class Test
    routine main() -> inout array{string} args, int result
        for string str in args
            print(str)
    end

    int main(char** argv, int argc) {
        for (int i = 0; i < argc; ++i)
            print(argv[i]);
    }
end

/*
    inout support
*/

foo() -> inout int i
    i += 3
end

int i2 = foo(int i1)
    i2 = i1 + 3
end

foo
    add 3, i
end

----------------

int a = 5
inout a = foo()

a1 = 5
a2 = foo(a1)

mov 5, a1
call foo

# loops and goto elimination

routine test()
    while someCond1 : outerLoop
        for int i in range(1, 4)
            if someCond2
                break
            if (someCond3)
                break outerLoop
        end
    else
        # This loop was breaked do some things
    end
end

---------------------------------------------------------------------------------------------------

for vec v in vecs.each()
    v + 3.f
end

simd: v + 3.f

for index i in vecs1.each()
    vecs1[i] += vecs2[i]
end

vecs1[@] += vecs2[@]

vecs[3, 21] += vecs[8, 26]


class Foo

# pointers

    routine foobar()
        ptr{Foo} foo # Foo* foo

        ptr{const Foo} # const Foo*
        ptr{const Foo} # const Foo*

        const ptr{Foo} # Foo const*
        const ptr{Foo} # Foo const*

    end

# functions

    reader foobar(Foo foo, int i, uint ui) -> real r, uint ui
    end

    routine test()
        Foo foo
        int i = 5

        uint ui = 7
        real r, ui = (bla + blub)[7].foo(5, 6, 7, inout i)

        Foo f, g = t, u, inout i
    end
end

# 
# constructors and assign operators
#

class Vec2

    # copy constructor
    create (Vec2 v)
        .x = v.x
        .y = v.y
    end

    # real constructor
    create (real x, real y)
    end

    assign (real x, real y, real z)
    end

    # assignment statements
    # 1.
    item1, item2 = funcExpr
        # 

    routine test()
        Vec2 v1 = 2.0, 3.0  # real constructor
        Vec2 v2 = v1        # copy constructor
        v1 = v2             # assign operator
        v1 = 2.0, 3.0       # error: no assign operator defined '=(real, real)'
        v1 = Vec2(2.0, 3.0) # assign operator
    end

    #
    # data
    # 

    real x
    real y
end

class Foo

    routine foo() -> simd{Vec3} result
        # ...
    end

    routine main()
        
        simd{Vec3} v1
        simd{Vec3} v2

        simd Vec3 bar = 5.0, 6.0, 7.0

        simd[0x, 10000x]: v1 = ::foo() + v2
        simd[0x, 10000x]: v1 = ::cross(v2, v3) + v2

        simd[0x, 10000x]: v1 = (::foo() + ::cross(v2, v3)) + (5 + 7)

    end

end

    tmp1 = foo()
    tmp2 = 5 + 7


