simd class vec2
    real x
    real y

    simd create (real x, real y)
        .x = x
        .y = y
    end

    simd writer = (real x, real y)
        .x = x
        .y = y
    end

    simd reader + (vec2 v) -> vec2 result
        result.x = .x + v.x
        result.y = .y + v.y
    end

    simd reader - (vec2 v) -> vec2 result
        result.x = .x - v.x
        result.y = .y - v.y
    end

    simd reader * (vec2 v) -> vec2 result
        result.x = .x * v.x
        result.y = .y * v.y
    end

    simd reader / (vec2 v) -> vec2 result
        result.x = .x / v.x
        result.y = .y / v.y
    end

    simd reader + (real r) -> vec2 result
        result.x = .x + r
        result.y = .y + r
    end

    simd reader - (real r) -> vec2 result
        result.x = .x - r
        result.y = .y - r
    end

    simd reader * (real r) -> vec2 result
        result.x = .x * r
        result.y = .y * r
    end

    simd reader / (real r) -> vec2 result
        result.x = .x / r
        result.y = .y / r
    end

    # dot product
    simd reader o (vec2 v) -> real result
        result = .x*v.x + .y*v.y
    end

    writer rand()
        .x = c_call real rand_float()
        .y = c_call real rand_float()
    end

    reader print()
        c_call print_float(.x)
        c_call print_float(.y)
    end

    simd reader to_complex() -> complex result
        result.r = .x
        result.i = .y
    end
end

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
    simd reader `cross (vec3 v) -> vec3 result
        real x = .y*v.z - .z*v.y 
        real y = .z*v.x - .x*v.z 
        real z = .x*v.y - .y*v.x 
    end

    # dot product
    simd reader `dot (vec3 v) -> real result
        result = .x*v.x + .y*v.y + .z*v.z
    end

    writer rand()
        .x = c_call real rand_float()
        .y = c_call real rand_float()
        .z = c_call real rand_float()
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

simd class complex
    real r
    real i

    simd create (real r, real i)
        .r = r
        .i = i
    end

    simd reader + (complex c) -> complex result
        result.r = .r + c.r
        result.i = .i + c.i
    end

    simd reader + (vec2 v) -> complex result
        result.r = .r + v.x
        result.i = .i + v.y
    end

    simd reader sq() -> complex result
        result.r = .r*.r - .i*.i
        result.i = 2.0 * .r*.i
    end

    simd reader abs_sq() -> real result
        result = .r*.r + .i*.i
    end

    simd reader to_vec2() -> vec2 result
        result.x = .r
        result.y = .i
    end

    writer rand()
        .r = c_call real rand_float()
        .i = c_call real rand_float()
    end

    reader print()
        c_call print_float(.r)
        c_call print_float(.i)
    end
end
