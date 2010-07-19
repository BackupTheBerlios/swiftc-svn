simd class vec3
    real x
    real y
    real z

    simd create (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    simd writer = (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    simd reader + (vec3 v) -> vec3 result
        result.x = .x + v.x
        result.y = .y + v.y
        result.z = .z + v.z
    end

    simd reader - (vec3 v) -> vec3 result
        result.x = .x - v.x
        result.y = .y - v.y
        result.z = .z - v.z
    end

    simd reader * (vec3 v) -> vec3 result
        result.x = .x * v.x
        result.y = .y * v.y
        result.z = .z * v.z
    end

    simd reader / (vec3 v) -> vec3 result
        result.x = .x / v.x
        result.y = .y / v.y
        result.z = .z / v.z
    end

    simd reader + (real r) -> vec3 result
        result.x = .x + r
        result.y = .y + r
        result.z = .z + r
    end

    simd reader - (real r) -> vec3 result
        result.x = .x - r
        result.y = .y - r
        result.z = .z - r
    end

    simd reader * (real r) -> vec3 result
        result.x = .x * r
        result.y = .y * r
        result.z = .z * r
    end

    simd reader / (real r) -> vec3 result
        result.x = .x / r
        result.y = .y / r
        result.z = .z / r
    end

    # cross product
    simd reader x (vec3 v) -> vec3 result
        real x = .y*v.z - .z*v.y 
        real y = .z*v.x - .x*v.z 
        real z = .x*v.y - .y*v.x 
    end

    # dot product
    simd reader o (vec3 v) -> real result
        result = .x*v.x + .y*v.y + .z*v.z
    end

    reader print()
        c_call print_float(.x)
        c_call print_float(.y)
        c_call print_float(.z)
    end
end

simd class ivec3
    int x
    int y
    int z

    writer = (vec3 v)
        .x = v.x.to_int()
        .y = v.y.to_int()
        .z = v.z.to_int()
    end

    reader print()
        c_call print_int(.x)
        c_call print_int(.y)
        c_call print_int(.z)
    end
end

class Phong

    # n -> normal
    # l -> light direction
    # v -> view direction
    # a -> ambient
    # d -> diffuse
    # s -> specular
    # sh -> shininess
    simd routine illuminate(vec3 n, vec3 l, vec3 v,\
                            vec3 a, vec3 d, real s, real sh) -> ivec3 rgb
        real nl = n o l           # cache dot product
        vec3 r = n * nl * 2.0 - l # reflection vector
        real rv = r o v           # cache dot product
        
        if nl < 0.0
            nl = -0.0
        end

        vec3 intensity = a + d * nl

        if rv > 0.0
            real sn = s
            int i = 0
            while i < sh.to_int()
                sn = sn * rv
                intensity = intensity + sn
                i = i + 1
            end
        end

        rgb = intensity * 255.0 + 0.5

        if (rgb.x > 255)
            rgb.x = 255
        end
        if (rgb.y > 255)
            rgb.y = 255
        end
        if (rgb.z > 255)
            rgb.z = 255
        end
    end

    routine main() -> int result
        simd{ivec3} res = 40000000x
        simd{vec3} n    = 40000000x
        simd{vec3} l    = 40000000x
        simd{vec3} v    = 40000000x
        simd{vec3} a    = 40000000x
        simd{vec3} d    = 40000000x
        simd{real} s    = 40000000x
        #simd{real} sh   = 40000000x

        simd i: 0x, 40000000x
            res@ = ::illuminate(n@, l@, v@, a@, d@, s@, simd 3.0)
        end

        result = 0
    end
end
