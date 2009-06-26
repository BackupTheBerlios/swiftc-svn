#include <vector>
struct Vec3 {
    float x;
    float y;
    float z;

    Vec3(float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    }







    Vec3 operator + (const Vec3& v2) { Vec3 result;
        result.x = x + v2.x;
        result.y = y + v2.y;
        result.z = z + v2.z;
        return result;
    }
    Vec3 operator + (float r) { Vec3 result;
        result.x = x + r;
        result.y = y + r;
        result.z = z + r;
        return result;
    }
    static Vec3 cross(const Vec3& v1, const Vec3& v2) { Vec3 result;
        result.x = v1.y * v2.z - v1.z * v2.y;
        result.y = v1.z * v2.x - v1.x * v2.z;
        result.z = v1.x * v2.y - v1.y * v2.x;
        return result;
    }
};
int main() {
    std::vector<Vec3> vecs1;
    std::vector<Vec3> vecs2;
    std::vector<Vec3> vecs3;

    for (size_t i = 0; i < vecs1.size(); ++i)
        vecs1[i] = vecs2[i] + vecs3[i];
    for (size_t i = 0; i < vecs1.size(); ++i)
        vecs1[i] = Vec3::cross(vecs2[i], vecs3[i]);
    for (size_t i = 0; i < vecs1.size(); ++i)
        vecs1[i] = vecs2[i] + static_cast<float>(i);

    return 0;
}
