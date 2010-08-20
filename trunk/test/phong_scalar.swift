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
            int N = sh.to_int()
            while i < N
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
        array{ivec3} res = 10000000x
        array{vec3} n    = 10000000x
        array{vec3} l    = 10000000x
        array{vec3} v    = 10000000x
        array{vec3} a    = 10000000x
        array{vec3} d    = 10000000x
        array{real} s    = 10000000x
        #array{real} sh   = 40000000x

        # init with random stuff
        index i = 0x
        while  i < 10000000x
            n[i].rand()
            l[i].rand()
            v[i].rand()
            a[i].rand()
            d[i].rand()
            s[i] = c_call real rand_float()

            i = i + 1x
        end

        # start phong
        c_call start_timer()
        i = 0x
        while i < 10000000x
            res[i] = ::illuminate(n[i], l[i], v[i], a[i], d[i], s[i], 3.0)
            i = i + 1x
        end
        c_call stop_timer()

        result = 0
    end
end
