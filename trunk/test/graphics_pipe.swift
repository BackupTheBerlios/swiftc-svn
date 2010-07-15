class Vec3
    real x
    real y
    real z

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

    reader print()
        c_call print_float(.x)
        c_call print_float(.y)
        c_call print_float(.z)
    end
end

class Mat4x4
    real e00; real e01; real e02; real e03
    real e10; real e11; real e12; real e13
    real e20; real e21; real e22; real e23
    real e30; real e31; real e32; real e33

    create (real e00, real e01, real e02, real e03,\
            real e10, real e11, real e12, real e13,\
            real e20, real e21, real e22, real e23,\
            real e30, real e31, real e32, real e33)
        .e00 = e00; .e01 = e01; .e02 = e02; .e03 = e03 
        .e10 = e10; .e11 = e11; .e12 = e12; .e13 = e13 
        .e20 = e20; .e21 = e21; .e22 = e22; .e23 = e23 
        .e30 = e30; .e31 = e31; .e32 = e32; .e33 = e33 
    end

    writer = (real e00, real e01, real e02, real e03,\
              real e10, real e11, real e12, real e13,\
              real e20, real e21, real e22, real e23,\
              real e30, real e31, real e32, real e33)
        .e00 = e00; .e01 = e01; .e02 = e02; .e03 = e03 
        .e10 = e10; .e11 = e11; .e12 = e12; .e13 = e13 
        .e20 = e20; .e21 = e21; .e22 = e22; .e23 = e23 
        .e30 = e30; .e31 = e31; .e32 = e32; .e33 = e33 
    end

    reader x (Vec3 v) -> Vec3 result
        real   w =  .e30*v.x + .e31*v.y + .e32*v.z + .e33
        result.x = (.e00*v.x + .e01*v.y + .e02*v.z + .e03) / w
        result.y = (.e10*v.x + .e11*v.y + .e12*v.z + .e13) / w
        result.z = (.e20*v.x + .e21*v.y + .e22*v.z + .e23) / w
    end
end

class test
    routine main() -> int result
        Mat4x4 m = 1.0, 0.0, 0.0, 3.0, \
                   0.0, 1.0, 0.0, 4.0, \
                   0.0, 0.0, 1.0, 7.0, \
                   0.0, 0.0, 0.0, 1.0;

        Vec3 v = 1.0, 2.0, 3.0

        (m x v).print()
    end
end
