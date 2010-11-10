simd class Vec3
    real x
    real y
    real z

    create ()
    end

    create (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    writer = (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    simd reader + (Vec3 v) -> Vec3 result
        result.x = .x + v.x
        result.y = .y + v.y
        result.z = .z + v.z
    end

    simd reader x(Vec3 v) -> Vec3 result
        result.x = .y * v.z - .z * v.y
        result.y = .z * v.x - .x * v.z
        result.z = .x * v.y - .y * v.x
    end

    simd reader o(Vec3 v) -> real result
        result = .x*v.x + .y*v.y + .z*v.z
    end

    reader print()
        c_call print_float(.x)
        c_call print_float(.y)
        c_call print_float(.z)
    end

    routine main() -> int result
        Vec3 v1 = 1.0, 2.0, 3.0
        Vec3 v2 = 4.0, 5.0, 6.0
        c_call print_float(v1 o v2)
        c_call println()
        (v1 x v2).print()

        result = 0
    end
end
