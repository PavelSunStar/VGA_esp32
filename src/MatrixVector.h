#pragma once
#include <math.h>

struct vec2 {
    float x, y;

    // === constructors ===
    constexpr vec2() : x(0), y(0) {}
    constexpr vec2(float v) : x(v), y(v) {}
    constexpr vec2(float _x, float _y) : x(_x), y(_y) {}

    // === unary ===
    constexpr vec2 operator+() const { return *this; }
    constexpr vec2 operator-() const { return vec2(-x, -y); }

    // === vec2 with vec2 ===
    constexpr vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
    constexpr vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
    constexpr vec2 operator*(const vec2& v) const { return vec2(x * v.x, y * v.y); }
    constexpr vec2 operator/(const vec2& v) const { return vec2(x / v.x, y / v.y); }

    // === vec2 with scalar ===
    constexpr vec2 operator+(float s) const { return vec2(x + s, y + s); }
    constexpr vec2 operator-(float s) const { return vec2(x - s, y - s); }
    constexpr vec2 operator*(float s) const { return vec2(x * s, y * s); }
    constexpr vec2 operator/(float s) const { return vec2(x / s, y / s); }

    // === compound ===
    vec2& operator+=(const vec2& v) { x += v.x; y += v.y; return *this; }
    vec2& operator-=(const vec2& v) { x -= v.x; y -= v.y; return *this; }
    vec2& operator*=(const vec2& v) { x *= v.x; y *= v.y; return *this; }
    vec2& operator/=(const vec2& v) { x /= v.x; y /= v.y; return *this; }

    vec2& operator+=(float s) { x += s; y += s; return *this; }
    vec2& operator-=(float s) { x -= s; y -= s; return *this; }
    vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    // === compare ===
    constexpr bool operator==(const vec2& v) const { return x == v.x && y == v.y; }
    constexpr bool operator!=(const vec2& v) const { return !(*this == v); }

    // === math ===
    constexpr float dot(const vec2& v) const { return x * v.x + y * v.y; }
    constexpr float cross(const vec2& v) const { return x * v.y - y * v.x; } // 2D pseudo-cross
    constexpr float lengthSq() const { return x * x + y * y; }
    float length() const { return sqrtf(lengthSq()); }

    vec2 normalized() const {
        float len = length();
        if (len <= 0.000001f) return vec2(0.0f);
        return vec2(x / len, y / len);
    }

    void normalize() {
        float len = length();
        if (len <= 0.000001f) { x = 0.0f; y = 0.0f; return; }
        x /= len;
        y /= len;
    }

    float distance(const vec2& v) const {
        return (*this - v).length();
    }

    constexpr float distanceSq(const vec2& v) const {
        return (*this - v).lengthSq();
    }

    constexpr vec2 perp() const { return vec2(-y, x); } // перпендикуляр

    static constexpr vec2 zero() { return vec2(0.0f, 0.0f); }
    static constexpr vec2 one()  { return vec2(1.0f, 1.0f); }
};

inline constexpr vec2 operator+(float s, const vec2& v) { return vec2(s + v.x, s + v.y); }
inline constexpr vec2 operator-(float s, const vec2& v) { return vec2(s - v.x, s - v.y); }
inline constexpr vec2 operator*(float s, const vec2& v) { return vec2(s * v.x, s * v.y); }
inline constexpr vec2 operator/(float s, const vec2& v) { return vec2(s / v.x, s / v.y); }


struct vec3 {
    float x, y, z;

    // === constructors ===
    constexpr vec3() : x(0), y(0), z(0) {}
    constexpr vec3(float v) : x(v), y(v), z(v) {}
    constexpr vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    constexpr vec3(float _x, const vec2& yz) : x(_x), y(yz.x), z(yz.y) {}

    // === unary ===
    constexpr vec3 operator+() const { return *this; }
    constexpr vec3 operator-() const { return vec3(-x, -y, -z); }

    // === vec3 with vec3 ===
    constexpr vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    constexpr vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    constexpr vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }
    constexpr vec3 operator/(const vec3& v) const { return vec3(x / v.x, y / v.y, z / v.z); }

    // === vec3 with scalar ===
    constexpr vec3 operator+(float s) const { return vec3(x + s, y + s, z + s); }
    constexpr vec3 operator-(float s) const { return vec3(x - s, y - s, z - s); }
    constexpr vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    constexpr vec3 operator/(float s) const { return vec3(x / s, y / s, z / s); }

    // === compound ===
    vec3& operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec3& operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec3& operator*=(const vec3& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    vec3& operator/=(const vec3& v) { x /= v.x; y /= v.y; z /= v.z; return *this; }

    vec3& operator+=(float s) { x += s; y += s; z += s; return *this; }
    vec3& operator-=(float s) { x -= s; y -= s; z -= s; return *this; }
    vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

    // === compare ===
    constexpr bool operator==(const vec3& v) const { return x == v.x && y == v.y && z == v.z; }
    constexpr bool operator!=(const vec3& v) const { return !(*this == v); }

    // === math ===
    constexpr float dot(const vec3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    constexpr vec3 cross(const vec3& v) const {
        return vec3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    constexpr float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return sqrtf(lengthSq()); }

    vec3 normalized() const {
        float len = length();
        if (len <= 0.000001f) return vec3(0.0f);
        return vec3(x / len, y / len, z / len);
    }

    void normalize() {
        float len = length();
        if (len <= 0.000001f) { x = y = z = 0.0f; return; }
        x /= len;
        y /= len;
        z /= len;
    }

    float distance(const vec3& v) const {
        return (*this - v).length();
    }

    constexpr float distanceSq(const vec3& v) const {
        return (*this - v).lengthSq();
    }

    constexpr vec2 xy() const { return vec2(x, y); }
    constexpr vec2 yz() const { return vec2(y, z); }
    constexpr vec2 xz() const { return vec2(x, z); }

    static constexpr vec3 zero() { return vec3(0.0f, 0.0f, 0.0f); }
    static constexpr vec3 one()  { return vec3(1.0f, 1.0f, 1.0f); }
};

inline constexpr vec3 operator+(float s, const vec3& v) { return vec3(s + v.x, s + v.y, s + v.z); }
inline constexpr vec3 operator-(float s, const vec3& v) { return vec3(s - v.x, s - v.y, s - v.z); }
inline constexpr vec3 operator*(float s, const vec3& v) { return vec3(s * v.x, s * v.y, s * v.z); }
inline constexpr vec3 operator/(float s, const vec3& v) { return vec3(s / v.x, s / v.y, s / v.z); }


// =========================
// utility helpers
// =========================
namespace Vector {

    inline float clamp(float v, float mn, float mx) {
        return (v < mn) ? mn : (v > mx ? mx : v);
    }

    inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    inline vec2 lerp(const vec2& a, const vec2& b, float t) {
        return a + (b - a) * t;
    }

    inline vec3 lerp(const vec3& a, const vec3& b, float t) {
        return a + (b - a) * t;
    }

    inline float radians(float deg) {
        return deg * 0.01745329251994329576923690768489f;
    }

    inline float degrees(float rad) {
        return rad * 57.295779513082320876798154814105f;
    }

    inline vec2 rotate(const vec2& v, float angleRad) {
        float s = sinf(angleRad);
        float c = cosf(angleRad);
        return vec2(
            v.x * c - v.y * s,
            v.x * s + v.y * c
        );
    }

    inline vec3 rotateX(const vec3& v, float angleRad) {
        float s = sinf(angleRad);
        float c = cosf(angleRad);
        return vec3(
            v.x,
            v.y * c - v.z * s,
            v.y * s + v.z * c
        );
    }

    inline vec3 rotateY(const vec3& v, float angleRad) {
        float s = sinf(angleRad);
        float c = cosf(angleRad);
        return vec3(
            v.x * c + v.z * s,
            v.y,
            -v.x * s + v.z * c
        );
    }

    inline vec3 rotateZ(const vec3& v, float angleRad) {
        float s = sinf(angleRad);
        float c = cosf(angleRad);
        return vec3(
            v.x * c - v.y * s,
            v.x * s + v.y * c,
            v.z
        );
    }

    inline vec3 reflect(const vec3& v, const vec3& n) {
        // n should be normalized
        return v - n * (2.0f * v.dot(n));
    }

    inline vec3 project(const vec3& v, const vec3& onto) {
        float d = onto.dot(onto);
        if (d <= 0.000001f) return vec3(0.0f);
        return onto * (v.dot(onto) / d);
    }
}