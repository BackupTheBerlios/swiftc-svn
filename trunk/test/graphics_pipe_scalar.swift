simd class Vec3
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

    reader print()
        c_call print_float(.x)
        c_call print_float(.y)
        c_call print_float(.z)
    end
end

simd class Mat4x4
    real e00; real e01; real e02; real e03
    real e10; real e11; real e12; real e13
    real e20; real e21; real e22; real e23
    real e30; real e31; real e32; real e33

    simd create (real e00, real e01, real e02, real e03,\
            real e10, real e11, real e12, real e13,\
            real e20, real e21, real e22, real e23,\
            real e30, real e31, real e32, real e33)
        .e00 = e00; .e01 = e01; .e02 = e02; .e03 = e03 
        .e10 = e10; .e11 = e11; .e12 = e12; .e13 = e13 
        .e20 = e20; .e21 = e21; .e22 = e22; .e23 = e23 
        .e30 = e30; .e31 = e31; .e32 = e32; .e33 = e33 
    end

    simd writer = (real e00, real e01, real e02, real e03,\
              real e10, real e11, real e12, real e13,\
              real e20, real e21, real e22, real e23,\
              real e30, real e31, real e32, real e33)
        .e00 = e00; .e01 = e01; .e02 = e02; .e03 = e03 
        .e10 = e10; .e11 = e11; .e12 = e12; .e13 = e13 
        .e20 = e20; .e21 = e21; .e22 = e22; .e23 = e23 
        .e30 = e30; .e31 = e31; .e32 = e32; .e33 = e33 
    end

    simd reader x (Vec3 v) -> Vec3 result
        real   w =  .e30*v.x + .e31*v.y + .e32*v.z + .e33
        result.x = (.e00*v.x + .e01*v.y + .e02*v.z + .e03) / w
        result.y = (.e10*v.x + .e11*v.y + .e12*v.z + .e13) / w
        result.z = (.e20*v.x + .e21*v.y + .e22*v.z + .e23) / w
    end

    writer rand()
        .e00 = c_call real rand_float()
        .e01 = c_call real rand_float()
        .e02 = c_call real rand_float()
        .e03 = c_call real rand_float()

        .e10 = c_call real rand_float()
        .e11 = c_call real rand_float()
        .e12 = c_call real rand_float()
        .e13 = c_call real rand_float()

        .e20 = c_call real rand_float()
        .e21 = c_call real rand_float()
        .e22 = c_call real rand_float()
        .e23 = c_call real rand_float()

        .e30 = c_call real rand_float()
        .e31 = c_call real rand_float()
        .e32 = c_call real rand_float()
        .e33 = c_call real rand_float()
    end
end

class test
    routine main() -> int result
        Mat4x4 m
        m.rand()
        array{Vec3} vecs = 40000000x

        c_call start_timer()
        index i = 0x
        while i < 40000000x
            vecs[i] = m x vecs[i]
            i = i + 1x
        end
        c_call stop_timer()
    end
end


