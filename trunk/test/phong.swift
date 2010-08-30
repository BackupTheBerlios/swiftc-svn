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
        real nl = n `dot l          # cache dot product
        vec3 r = n * nl * 2.0 - l # reflection vector
        real rv = r `dot v           # cache dot product
        
        if nl < 0.0
            nl = -0.0
        end

        vec3 intensity = a + d * nl

        if rv > 0.0
            real sn = s
            int i = 0

            #while i < sh.to_int()
            #    sn = sn * rv
            #    intensity = intensity + sn
            #    i = i + 1
            #end
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
        simd{ivec3} res = 10000000x
        simd{vec3} n    = 10000000x
        simd{vec3} l    = 10000000x
        simd{vec3} v    = 10000000x
        simd{vec3} a    = 10000000x
        simd{vec3} d    = 10000000x
        simd{real} s    = 10000000x
        #simd{real} sh   = 40000000x

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
        simd i: 0x, 10000000x
            res@ = ::illuminate(n@, l@, v@, a@, d@, s@, simd 3.0)
        end
        c_call stop_timer()

        result = 0
    end
end
