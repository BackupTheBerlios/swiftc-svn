
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

    assign (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    simd operator + (Vec3 v1, Vec3 v2) -> Vec3 result
        result.x = v1.x + v2.x
        result.y = v1.y + v2.y
        result.z = v1.z + v2.z
    end

    simd operator + (Vec3 v1, real r) -> Vec3 result
        result.x = v1.x + r
        result.y = v1.y + r
        result.z = v1.z + r
    end

    simd routine cross(Vec3 v1, Vec3 v2) -> Vec3 result
        result.x = v1.y * v2.z - v1.z * v2.y
        result.y = v1.z * v2.x - v1.x * v2.z
        result.z = v1.x * v2.y - v1.y * v2.x
    end

    routine main() -> int result
        Vec3 v1 =  1.0, 2.0,  3.0
        Vec3 v2 = -2.0, 1.5, -3.0
        # Vec3 v3 = Vec3::cross(v1, v2) # yields -10.5, -3, 5.5
        Vec3 v3
        v3 = v1 + v2
        v3 = ::cross(v1 + v2, v2)

        c_call print_float(v3.x)
        c_call print_float(v3.y)
        c_call print_float(v3.z)

        result = 0

        simd{Vec3} vecs1 = 10000x
        simd{Vec3} vecs2 = 10000x
        simd{Vec3} vecs3 = 10000x

        simd[0x, 10000x]: vecs1 = vecs2 + vecs3
        simd[0x, 10000x]: vecs1 = Vec3::cross(vecs2, vecs3)
        # simd[0x, 10000x]: c_call print_float(v3.z)
        # simd: vecs1 = vecs2 + @.to_real()
    end

end
