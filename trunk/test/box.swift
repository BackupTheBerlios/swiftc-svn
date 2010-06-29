simd class Vec3
    real x
    real y
    real z

    create (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end
end

simd class Box
    Vec3 urf
    Vec3 llb

    create (Vec3 urf, Vec3 llb)
        .urf = urf
        .llb = llb
    end

    assign = (Vec3 urf, Vec3 llb)
        .urf = urf
        .llb = llb
    end

    reader print()
        c_call print_float(.urf.x)
        c_call print_float(.urf.y)
        c_call print_float(.urf.z)
        c_call print_float(.llb.x)
        c_call print_float(.llb.y)
        c_call print_float(.llb.z)
    end
end

class Foo
    routine main() -> int result
        simd{Box} boxes = 100x

        scope 
            Vec3 urf = 1.0, 2.0, 3.0
            Vec3 llb = 4.0, 5.0, 6.0
            Box box = urf, llb
            boxes[77x] = box
        end

        scope 
           Box box = boxes[77x]
           box:print()
        end
    end
end
