class Vec3
    real x
    real y
    real z

    create (Vec3 v)
        .x = v.x
        .y = v.y
        .z = v.z
    end

    create (real x, real y, real z)
        .x = x
        .y = y
        .z = z
    end

    writer = (Vec3 v)
        .x = v.x
        .y = v.y
        .z = v.z
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

    routine beer(real r) -> Vec3 v1, Vec3 v2
        v1.x = 1.0 + r
        v1.y = 2.0 + r
        v1.z = 3.0 + r
        v2.x = 4.0 + r
        v2.y = 5.0 + r
        v2.z = 6.0 + r
    end

    routine main() -> int result
        scope
            Vec3 v, Vec3 w = ::beer(0.0)
            v.print()
            w.print()

            c_call println()

            v, w = ::beer(1.0)
            v.print()
            w.print()
        end

        c_call println()

        scope
            Vec3 v = 3.0, 4.0, 5.0
            v.print()
            v = 6.0, 7.0, 8.0
            v.print()
        end

        c_call println()

        real r = 5.0 / 2.0
        c_call print_float(r)

        result = 0
    end
end
