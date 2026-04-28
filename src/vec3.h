#pragma once
#include <cmath>
#include <ostream>

class Vec3 {
public:
    double x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator*=(double t)      { x *= t;   y *= t;   z *= t;   return *this; }
    Vec3& operator/=(double t)      { return *this *= 1.0 / t; }

    double lengthSquared() const { return x*x + y*y + z*z; }
    double length()        const { return std::sqrt(lengthSquared()); }

    bool nearZero() const {
        constexpr double s = 1e-8;
        return std::abs(x) < s && std::abs(y) < s && std::abs(z) < s;
    }
};

// Arithmetic operators
inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3 operator*(const Vec3& a, const Vec3& b) { return {a.x*b.x, a.y*b.y, a.z*b.z}; }
inline Vec3 operator*(double t, const Vec3& v)      { return {t*v.x, t*v.y, t*v.z}; }
inline Vec3 operator*(const Vec3& v, double t)      { return t * v; }
inline Vec3 operator/(const Vec3& v, double t)      { return (1.0/t) * v; }

inline double dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

inline Vec3 normalize(const Vec3& v) { return v / v.length(); }
inline Vec3 reflect(const Vec3& v, const Vec3& n) { return v - 2*dot(v,n)*n; }

// Type aliases
using Point3 = Vec3;
using Color  = Vec3;
