
simd class Vec3
    real x
    real y
    real z

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
        result.x = v1.y * v2.z - v1.z * v2.x
        result.y = v1.z * v2.x - v1.x * v2.z
        result.z = v1.x * v2.y - v1.y * v2.x

    end

    routine main() -> int result
        simd{Vec3} vecs1
        simd{Vec3} vecs2
        simd{Vec3} vecs3


        simd: vecs1 = vecs2 + vecs3

        simd: vecs1 = Vec3::cross(vecs2, vecs3)

        simd: vecs1 = vecs2 + @.to_real()

        result = 0
    end
end
