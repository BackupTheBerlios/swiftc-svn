#ifndef TEST_VECTOR_H
#define TEST_VECTOR_H

struct Vec3 
{
    float x;
    float y;
    float z;

    Vec3() {}
    Vec3(float _x, float _y, float _z) 
    {
        x = _x;
        y = _y;
        z = _z;
    }

    Vec3 operator + (const Vec3& v2);
    Vec3 operator + (float r);
    static Vec3 cross(const Vec3& v1, const Vec3& v2);
};

#endif // TEST_VECTOR_H
