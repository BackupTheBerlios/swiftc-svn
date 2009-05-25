simd class Vec3
    real x
    real y
    real z

    simd routine cross(Vec3 v1, Vec3 v2) -> Vec3 result
        result.x = v1.y * v2.z - v1.z * v2.x
        result.y = v1.z * v2.x - v1.x * v2.z
        result.z = v1.x * v2.y - v1.y * v2.x
    end

    simd operator + (Vec3 v1, Vec3 v2) -> Vec3 result
        x = v1.x + v2.x
        y = v1.y + v2.y
        z = v1.z + v2.z
    end

    routine main() -> int result
        simd{Vec3} vecs1
        simd{Vec3} vecs2
        simd{Vec3} vecs3

        simd: vecs1 = vec2 + vecs3
        simd: vecs1 = Vec3::cross(vecs2, vecs3)
        # simd: vecs1 = v1 + %i.to_real()

        result = 0
    end
end
