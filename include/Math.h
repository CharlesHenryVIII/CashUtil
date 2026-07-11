#pragma once
#include "gb_math.h"
#include "Debug.h"

#include <cmath>
#include <cstdlib>
#include <vector>
#include <cstdint>

#define BIT(num) (1<<(num))
#define HasBit(n, pos) ((n) & (1 << (pos)))
#define MATH_PREFIX [[nodiscard]] inline

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

#define Thousands(value) ((value) * 1000LL)
#define Millions(value)  (Thousands(value) * 1000LL)
#define Billions(value)  (Millions(value) * 1000LL)
#define Trillions(value) (Billions(value) * 1000LL)

#define ToThousands(value) ((u64)((value) / 1000LL))
#define ToMillions(value)  ((u64)(ToThousands(value) / 1000LL))
#define ToBillions(value)  ((u64)(ToMillions(value) / 1000LL))
#define ToTrillions(value) ((u64)(ToBillions(value) / 1000LL))

#define MilliFromSeconds(value)         Thousands(value)
#define MicroFromMilliseconds(value)    Thousands(value)
#define MicroFromSeconds(value)         Millions(value)
#define MilliFromMicroseconds(value)    ((value) / 1000LL)
#define SecFromMicroseconds(value)      ((value) / 1000000LL)

#define Kilobytes(value) (         (value) * 1000LL)
#define Megabytes(value) (Kilobytes(value) * 1000LL)
#define Gigabytes(value) (Megabytes(value) * 1000LL)
#define Terabytes(value) (Gigabytes(value) * 1000LL)

#define ToKilobytesInt(value) ((u64)(              (value) / 1000.0))
#define ToMegabytesInt(value) ((u64)(ToKilobytesInt(value) / 1000.0))
#define ToGigabytesInt(value) ((u64)(ToMegabytesInt(value) / 1000.0))
#define ToTerabytesInt(value) ((u64)(ToGigabytesInt(value) / 1000.0))

#define ToKilobytesFloat(value) (           (value) / 1000.0)
#define ToMegabytesFloat(value) (ToKilobytesFloat(value) / 1000.0)
#define ToGigabytesFloat(value) (ToMegabytesFloat(value) / 1000.0)
#define ToTerabytesFloat(value) (ToGigabytesFloat(value) / 1000.0)

#define Kibibyte(value)  (         (value) * 1024L)
#define Mebibytes(value) (Kilobytes(value) * 1024L)
#define Gibibytes(value) (Megabytes(value) * 1024L)
#define Tebibytes(value) (Gigabytes(value) * 1024L)

#define ToKibibytes(value) ((u64)(           (value) / 1024.0))
#define ToMebibytes(value) ((u64)(ToKilobytes(value) / 1024.0))
#define ToGibibytes(value) ((u64)(ToMegabytes(value) / 1024.0))
#define ToTebibytes(value) ((u64)(ToGigabytes(value) / 1024.0))

typedef gbVec4<float> Color;

union ColorInt {
    u32 rgba;
    struct { u8 r, g, b, a; };
    u8 e[4];
};

MATH_PREFIX Color ToColor(ColorInt c)
{
    Color r;
    r.r = c.r / float(UCHAR_MAX);
    r.g = c.g / float(UCHAR_MAX);
    r.b = c.b / float(UCHAR_MAX);
    r.a = c.a / float(UCHAR_MAX);
    return r;
}

const Color Red         = { 1.00f, 0.00f, 0.00f, 1.00f };
const Color Green       = { 0.00f, 1.00f, 0.00f, 1.00f };
const Color Blue        = { 0.00f, 0.00f, 1.00f, 1.00f };
const Color transRed    = { 1.00f, 0.00f, 0.00f, 0.50f };
const Color transGreen  = { 0.00f, 1.00f, 0.00f, 0.50f };
const Color transBlue   = { 0.00f, 0.00f, 1.00f, 0.50f };
const Color transOrange = { 1.00f, 0.50f, 0.00f, 0.50f };
const Color transPurple = { 1.00f, 0.00f, 1.00f, 0.50f };
const Color Purple      = { 1.00f, 0.00f, 1.00f, 1.00f };
const Color lightRed    = { 1.00f, 0.00f, 0.00f, 0.25f };
const Color lightGreen  = { 0.00f, 1.00f, 0.00f, 0.25f };
const Color lightBlue   = { 0.00f, 0.00f, 1.00f, 0.25f };
const Color White       = { 1.00f, 1.00f, 1.00f, 1.00f };
const Color lightWhite  = { 0.58f, 0.58f, 0.58f, 0.58f };
const Color Black       = { 0.00f, 0.00f, 0.00f, 1.00f };
const Color lightBlack  = { 0.00f, 0.00f, 0.00f, 0.58f };
const Color Brown       = { 0.50f, 0.40f, 0.25f, 1.00f };
const Color Mint        = { 0.00f, 1.00f, 0.50f, 1.00f };
const Color Orange      = { 1.00f, 0.50f, 0.00f, 1.00f };
const Color Grey        = { 0.50f, 0.50f, 0.50f, 1.00f };
const Color transCyan   = { 0.60f, 0.60f, 1.00f, 0.50f };
const Color Yellow      = { 1.00f, 1.00f, 0.00f, 1.00f };

const Color backgroundColor = { 0.263f, 0.706f, 0.965f, 0.0f };

const Color HealthBarBackground = { 0.25f, 0.33f, 0.25f, 1.0f};
constexpr i32 blockSize = 32;

constexpr float pi = 3.14159f;
constexpr float half_pi = pi / 2.0f;
constexpr float tau = 2 * pi;
const float inf = INFINITY;

typedef gbVec2<float>   Vec2;
typedef gbVec3<float>   Vec3;
typedef gbVec4<float>   Vec4;
typedef gbMat2<float>   Mat2;
typedef gbMat3<float>   Mat3;
typedef gbMat4<float>   Mat4;
typedef gbMat4x3<float> Mat4x3;
typedef gbQuat<float>   Quat;
typedef gbVec2<double>  Vec2d;
typedef gbVec3<double>  Vec3d;
typedef gbVec4<double>  Vec4d;
typedef gbMat2<double>  Mat2d;
typedef gbMat3<double>  Mat3d;
typedef gbMat4<double>  Mat4d;
typedef gbMat4x3<double>Mat4x3d;
typedef gbQuat<double>  Quatd;
typedef gbVec2<i32>     Vec2I;
typedef gbVec3<i32>     Vec3I;
typedef gbVec4<i32>     Vec4I;
typedef gbVec2<u32>     Vec2U;
typedef gbVec3<u32>     Vec3U;
typedef gbVec4<u32>     Vec4U;
typedef gbMat2<i32>     Mat2I;
typedef gbMat3<i32>     Mat3I;
typedef gbMat4<i32>     Mat4I;

MATH_PREFIX Vec4 GetVec4(Vec3 a, float b)
{
    return { a.x, a.y, a.z, b };
}

union U32Pack {
    u32 pack;
    u32 rgba;
    u32 xyzw;
    struct { u8 r, g, b, a; };
    struct { u8 x, y, z, w; };
    u8 e[4];
};

//TODO(CSH): rename these to PTN, PNTC, PNC, etc (position, normal, texture, color)
#pragma pack(push, 1)
struct Vertex {
    Vec3 p;
    Vec2 uv;
    Vec3 n;
};
struct Vertex_Cube {
    Vec3 p;
    Vec3 n;
    Vec2 uv;
    Color color;
};
struct Vertex_Tetra {
    Color color;
    Vec3 p;
    Vec3 n;
};
struct Vertex_Gltf {
    Vec3 p;
    Vec3 n;
    Vec2 uv;
};
struct Vertex_Line {
    Vec3 p;
    Color color;
};
struct Vertex_Font {
    Vec2 p;
    Color color;
    Vec2 uv;
};
struct Vertex_Simple {
    Vec3 p;
    Vec3 n;
};

#pragma pack(pop)

struct RectInt {
    Vec2I botLeft = {};
    Vec2I topRight = {};

    i32 Width()
    {
        return topRight.x - botLeft.x;
    }

    i32 Height()
    {
        return topRight.y - botLeft.y;
    }

};

struct Rect {
    Vec2 botLeft = {};
    Vec2 topRight = {};

    float Width()
    {
        return topRight.x - botLeft.x;
    }

    float Height()
    {
        return topRight.y - botLeft.y;
    }

};

struct LineSegment {
    Vec2 p0;
    Vec2 p1;

    Vec2 Normal()
    {
        //TODO CHECK IF THIS IS RIGHT
        Vec2 result = (p1 - p0);
        result = { -result.y, result.x };
        return result;
    }
};


template <typename T = float>
struct Range {
    T min, max;

//    float RandomInRange()
//    {
//        return Random<float>(min, max);
//    }
//    void AngleSymetric(float angle, float range)
//    {
//        min = angle - range / 2;
//        max = angle + range / 2;
//    }
    //[[nodiscard]] inline T Center()
    MATH_PREFIX T Center() const
    {
        return min + ((max - min) / 2);
    }
};


//struct Rectangle {
//    Vec2 botLeft;
//    Vec2 topRight;
//
//    bool Collision(Vec2I loc)
//    {
//        bool result = false;
//        if (loc.y > botLeft.x && loc.y < topRight.x)
//            if (loc.x > botLeft.x && loc.x < topRight.x)
//                result = true;
//        return result;
//    }
//};

struct Rectangle_Int {
    Vec2I bottomLeft;
    Vec2I topRight;

    i32 Width() const
    {
        return topRight.x - bottomLeft.x;
    }

    i32 Height() const
    {
        return topRight.y - bottomLeft.y;
    }
};

struct Transform {
    Vec3 pos = {};
    Quat rot = gb_quat_identity<float>();
    //Transform() = default;
    //Transform(const Vec3& p, const Quat& r) : pos(p), rot(r) {};
};
struct STransform {
    Vec3 pos = {};
    Quat rot = gb_quat_identity<float>();
    Vec3 scale = {1, 1, 1};
    //STransform() = default;
    //STransform(const Vec3& p, const Quat& r, const Vec3& s) : Transform(p, r), scale(s) {};
};

MATH_PREFIX STransform TransformMul(const STransform p, const STransform c)
{
    STransform r;
    r.pos = p.pos + (p.rot * (p.scale * c.pos));
    r.rot = p.rot * c.rot;
    r.scale = p.scale * c.scale;
    return r;
}
MATH_PREFIX STransform operator* (const STransform& a, const STransform& b) { return TransformMul(a, b); }

MATH_PREFIX Transform* AsTransform(STransform* t) { return (Transform*)t; }
MATH_PREFIX const Transform* AsTransform(const STransform* t) { return (Transform*)t; }

inline void ValidateTransform(STransform& transform)
{
    if (transform.rot == Quat(0, 0, 0, 0))
        transform.rot = gb_quat_identity();
    if (transform.scale.x <= 0)
        transform.scale.x = 1;
    if (transform.scale.y <= 0)
        transform.scale.y = 1;
    if (transform.scale.z <= 0)
        transform.scale.z = 1;
}


MATH_PREFIX Vec4  ToVec4(const Vec3I& a, const int b)
{ return { static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z), static_cast<float>(b) }; }
MATH_PREFIX Vec4  ToVec4(const Vec2I& a, const Vec2I& b)
{ return { static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(b.x), static_cast<float>(b.y) }; }
MATH_PREFIX Vec4  ToVec4(const Vec3& a, const float b)
{ return { a.x, a.y, a.z, b }; }
MATH_PREFIX Vec4  ToVec4(const Vec2& a, const Vec2& b)
{ return { a.x, a.y, b.x, b.y }; }
MATH_PREFIX Vec3I ToVec3I(const Vec3& a)
{ return { static_cast<i32>(a.x), static_cast<i32>(a.y), static_cast<i32>(a.z) }; }
MATH_PREFIX Vec3I ToVec3I(const Vec2& a, const float b)
{ return { static_cast<i32>(a.x), static_cast<i32>(a.y), static_cast<i32>(b) }; }
MATH_PREFIX Vec3I ToVec3I(const Vec2I& a, const int b)
{ return { static_cast<i32>(a.x), static_cast<i32>(a.y), static_cast<i32>(b) }; }
MATH_PREFIX Vec3  ToVec3(const Vec2& a, float b)
{ return { static_cast<float>(a.x), static_cast<float>(a.y), b }; }
MATH_PREFIX Vec3  ToVec3(const Vec3I& a)
{ return { static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z) }; }
MATH_PREFIX Vec2I ToVec2I(const Vec2& a)
{ return { static_cast<i32>(a.x), static_cast<i32>(a.y) }; }
MATH_PREFIX Vec2  ToVec2(const Vec2I& a)
{ return { static_cast<float>(a.x), static_cast<float>(a.y) }; }

template<typename T> inline bool operator> (gbVec2<T> a, gbVec2<T> b) { return (a.x >  b.x) && (a.y >  b.y); }
template<typename T> inline bool operator>=(gbVec2<T> a, gbVec2<T> b) { return (a.x >= b.x) && (a.y >= b.y); }
template<typename T> inline bool operator< (gbVec2<T> a, gbVec2<T> b) { return (a.x <  b.x) && (a.y <  b.y); }
template<typename T> inline bool operator<=(gbVec2<T> a, gbVec2<T> b) { return (a.x <= b.x) && (a.y <= b.y); }
template<typename T> inline bool operator> (gbVec3<T> a, gbVec3<T> b) { return (a.x >  b.x) && (a.y >  b.y) && (a.z >  b.z); }
template<typename T> inline bool operator>=(gbVec3<T> a, gbVec3<T> b) { return (a.x >= b.x) && (a.y >= b.y) && (a.z >= b.z); }
template<typename T> inline bool operator< (gbVec3<T> a, gbVec3<T> b) { return (a.x <  b.x) && (a.y <  b.y) && (a.z <  b.z); }
template<typename T> inline bool operator<=(gbVec3<T> a, gbVec3<T> b) { return (a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z); }
template<typename T> inline bool operator> (gbVec4<T> a, gbVec4<T> b) { return (a.x >  b.x) && (a.y >  b.y) && (a.z >  b.z) && (a.w >  a.w); }
template<typename T> inline bool operator>=(gbVec4<T> a, gbVec4<T> b) { return (a.x >= b.x) && (a.y >= b.y) && (a.z >= b.z) && (a.w >= a.w); }
template<typename T> inline bool operator< (gbVec4<T> a, gbVec4<T> b) { return (a.x <  b.x) && (a.y <  b.y) && (a.z <  b.z) && (a.w <  a.w); }
template<typename T> inline bool operator<=(gbVec4<T> a, gbVec4<T> b) { return (a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z) && (a.w <= a.w); }

template<typename T>
inline gbMat3<T> ToMat3(const gbMat4<T>& mat)
{
    gbMat3<T> r;
    r.x = mat.x.xyz;
    r.y = mat.y.xyz;
    r.z = mat.z.xyz;
    return r;
}

template<typename T>
inline gbMat3<T> ToMat3(const gbMat4x3<T>& mat)
{
    gbMat3<T> r;
    r.x = mat.x.xyz;
    r.y = mat.y.xyz;
    r.z = mat.z.xyz;
    return r;
}


MATH_PREFIX Vec2I operator*(const Vec2I& a, const float b)
{
    return { int(a.x * b),  int(a.y * b) };
}

//inline Vec2 operator/(const Vec2& a, const float b)
//{
//    return { a.x / b,  a.y / b };
//}
//inline Vec2 operator/(const float lhs, const Vec2& rhs)
//{
//    return { lhs / rhs.x,  lhs / rhs.y };
//}
//
//inline bool operator==(const Rectangle& lhs, const Rectangle& rhs)
//{
//    bool bl = lhs.botLeft == rhs.botLeft;
//    bool tr = lhs.topRight == rhs.topRight;
//    return bl && tr;
//}
//
//inline bool operator!=(const Rectangle& lhs, const Rectangle& rhs)
//{
//    return !(lhs == rhs);
//}
MATH_PREFIX Vec3 operator+(Vec3 a, float b)
{
    Vec3 r = {a.x + b, a.y + b, a.z + b};
    return r;
}
MATH_PREFIX Vec3 operator-(Vec3 a, float b)
{
    Vec3 r = {a.x - b, a.y - b, a.z - b};
    return r;
}
inline void operator+=(Vec3& a, float b)
{
    a = {a.x + b, a.y + b, a.z + b};
}
inline void operator-=(Vec3& a, float b)
{
    a = {a.x - b, a.y - b, a.z - b};
}

template <typename T>
MATH_PREFIX T Min(const T a, const T b)
{
    return a < b ? a : b;
}

 template <typename T>
MATH_PREFIX T Max(const T a, const T b)
{
    return a > b ? a : b;
}

template <typename T>
MATH_PREFIX T Clamp(const T v, const T min, const T max)
{
    return Max(min, Min(max, v));
}

MATH_PREFIX Vec2 Floor(const Vec2& v)
{
    return { floorf(v.x), floorf(v.y) };
}
MATH_PREFIX Vec3 Floor(const Vec3& v)
{
    return { floorf(v.x), floorf(v.y), floorf(v.z) };
}
MATH_PREFIX Vec4 Floor(const Vec4& v)
{
    return { floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w)  };
}

MATH_PREFIX Vec3 Trunc(const Vec3& v)
{
    return { truncf(v.x), truncf(v.y), truncf(v.z) };
}
MATH_PREFIX Vec4 Trunc(const Vec4& v)
{
    return { truncf(v.x), truncf(v.y), truncf(v.z), truncf(v.w)  };
}

MATH_PREFIX float Ceiling(const float& v)
{
    return ceilf(v);
}
MATH_PREFIX Vec2 Ceiling(const Vec2& v)
{
    return { ceilf(v.x), ceilf(v.y) };
}
MATH_PREFIX Vec3 Ceiling(const Vec3& v)
{
    return { ceilf(v.x), ceilf(v.y), ceilf(v.z) };
}
MATH_PREFIX Vec4 Ceiling(const Vec4& v)
{
    return { ceilf(v.x), ceilf(v.y), ceilf(v.z), ceilf(v.w)  };
}

MATH_PREFIX float Fract(float a)
{
    return a - floorf(a);
}
MATH_PREFIX Vec2 Fract(const Vec2& a)
{
    return a - Floor(a);
}
MATH_PREFIX Vec3 Fract(const Vec3& a)
{
    return a - Floor(a);
}
MATH_PREFIX Vec4 Fract(const Vec4& a)
{
    return a - Floor(a);
}

MATH_PREFIX float Abs(float a)
{
    return fabs(a);
}
MATH_PREFIX Vec3 Abs(const Vec3& a)
{
    return { fabs(a.x), fabs(a.y), fabs(a.z) };
}
MATH_PREFIX Vec2 Abs(const Vec2& a)
{
    return { abs(a.x), abs(a.y) };
}
MATH_PREFIX Vec3I Abs(const Vec3I& a)
{
    return { abs(a.x), abs(a.y), abs(a.z) };
}
MATH_PREFIX Vec2I Abs(const Vec2I& a)
{
    return { abs(a.x), abs(a.y) };
}

MATH_PREFIX Vec2 Sine(const Vec2& v)
{
    Vec2 r = { sinf(v.x), sinf(v.y) };
    return r;
}
MATH_PREFIX Vec3 Sine(const Vec3& v)
{
    Vec3 r = { sinf(v.x), sinf(v.y), sinf(v.z) };
    return r;
}

//MATH AND CONVERSIONS

MATH_PREFIX float RadToDeg(float angle)
{
    return ((angle) / (tau)) * 360;
}

MATH_PREFIX float DegToRad(float angle)
{
    return (angle / 360 ) * (tau);
}

//TODO: Improve
MATH_PREFIX Vec3 NormalizeZero(const Vec3& v)
{
    float prod = v.x * v.x + v.y * v.y + v.z * v.z;
    if (prod == 0.0f)
        return {};
    float hyp = sqrtf(prod);
    Vec3 result = { (v.x / hyp), (v.y / hyp), (v.z / hyp)};
    return result;
}
MATH_PREFIX Vec2 NormalizeZero(const Vec2& v)
{
    float prod = v.x * v.x + v.y * v.y;
    if (prod == 0.0f)
        return {};
    float hyp = sqrtf(prod);
    Vec2 result = { (v.x / hyp), (v.y / hyp) };
    return result;
}
template<typename T>
MATH_PREFIX gbVec2<T> Normalize(const gbVec2<T>& v)
{
    return gb_vec2_norm(v);
}
template<typename T>
MATH_PREFIX gbVec3<T> Normalize(const gbVec3<T>& v)
{
    return gb_vec3_norm(v);
}
template<typename T>
MATH_PREFIX gbVec4<T> Normalize(const gbVec4<T>& v)
{
    return gb_vec4_norm(v);
}
MATH_PREFIX float* Normalize(float* v, size_t length)
{
    float total = 0;
    for (size_t i = 0; i < length; i++)
    {
        total += (v[i] * v[i]);
    }
    float hyp = sqrtf(total);
    for (size_t i = 0; i < length; i++)
    {
        v[i] = v[i] / hyp;
    }
    return v;
}
MATH_PREFIX double* Normalize(double* v, size_t length)
{
    double total = 0;
    for (size_t i = 0; i < length; i++)
    {
        total += (v[i] * v[i]);
    }
    double hyp = sqrt(total);
    for (size_t i = 0; i < length; i++)
    {
        v[i] = v[i] / hyp;
    }
    return v;
}

template <typename T>
MATH_PREFIX T Lerp(const T& a, const T& b, float t)
{
    return a + (b - a) * t;
}

MATH_PREFIX Vec3 Lerp(const Vec3& a, const Vec3& b, const Vec3& t)
{
    return a + (b - a) * t;
}

MATH_PREFIX Vec3 Converge(const Vec3& value, const Vec3& target, float rate, float dt)
{
    return Lerp(target, value, exp2(-rate * dt));
}

#if 1
[[nodiscard]] float Bilinear(float p00, float p10, float p01, float p11, float x, float y);
#else
[[nodiscard]] float Bilinear(Vec2 p, Rect loc, float bl, float br, float tl, float tr);
#endif
[[nodiscard]] float Cubic(Vec4 v, float x);
[[nodiscard]] float Bicubic(Mat4 p, Vec2 pos);

/*
Atan2f return value:

3pi/4      pi/2        pi/4


            O
+/-pi      /|\          0
           / \


-3pi/4    -pi/2        pi/4
*/

template<typename T>
MATH_PREFIX T DotProduct(const gbVec2<T>& a, const gbVec2<T>& b)
{
    return a.x * b.x + a.y * b.y;
}
template<typename T>
MATH_PREFIX T DotProduct(const gbVec3<T>& a, const gbVec3<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
template<typename T>
MATH_PREFIX gbVec3<T> CrossProduct(const gbVec3<T>& a, const gbVec3<T>& b)
{
    return gb_vec3_cross(a, b);
}
template<typename T>
MATH_PREFIX T CrossProduct(const gbVec2<T>& a, const gbVec2<T>& b)
{
    return gb_vec2_cross(a, b);
}

template<typename T>
MATH_PREFIX T PythagsInner(const gbVec2<T>& a)
{
    return (a.x * a.x) + (a.y * a.y);
}
template<typename T>
MATH_PREFIX T PythagsInner(const gbVec3<T>& a)
{
    return (a.x * a.x) + (a.y * a.y) + (a.z * a.z);
}
template<typename T>
MATH_PREFIX T PythagsInner(const gbVec4<T>& a)
{
    return (a.x * a.x) + (a.y * a.y) + (a.z * a.z) + (a.w * a.w);
}
template<typename T>
MATH_PREFIX T Pythags(const gbVec2<T>& a)
{
    return sqrtf(PythagsInner(a));
}
template<typename T>
MATH_PREFIX T Pythags(const gbVec3<T>& a)
{
    return sqrtf(PythagsInner(a));
}
template<typename T>
MATH_PREFIX T Pythags(const gbVec4<T>& a)
{
    return sqrtf(PythagsInner(a));
}

template<typename T>
MATH_PREFIX T DistanceSquared(const gbVec2<T>& a)
{
    return PythagsInner(a);
}
template<typename T>
MATH_PREFIX T DistanceSquared(const gbVec3<T>& a)
{
    return PythagsInner(a);
}
template<typename T>
MATH_PREFIX T DistanceSquared(const gbVec4<T>& a)
{
    return PythagsInner(a);
}
template<typename T>
MATH_PREFIX T Distance(const gbVec2<T>& a, const gbVec2<T>& b)
{
    return Pythags(a - b);
}
template<typename T>
MATH_PREFIX T Distance(const gbVec3<T>& a, const gbVec3<T>& b)
{
    return Pythags(a - b);
}
template<typename T>
MATH_PREFIX T Distance(const gbVec4<T>& a, const gbVec4<T>& b)
{
    return Pythags(a - b);
}

template<typename T>
MATH_PREFIX T Length(const gbVec2<T>& a)
{
    return Pythags<T>(a);
}
MATH_PREFIX float Length(const Vec2I& a)
{
    return Pythags(ToVec2(a));
}
template<typename T>
MATH_PREFIX T Length(const gbVec3<T>& a)
{
    return Pythags<T>(a);
}
MATH_PREFIX float Length(const Vec3I& a)
{
    return Pythags(ToVec3(a));
}

MATH_PREFIX Vec3 Acos(const Vec3& a)
{
    return { acos(a.x), acos(a.y), acos(a.z) };
}

MATH_PREFIX Vec2 Round(const Vec2& a)
{
    Vec2 r = { roundf(a.x), roundf(a.y) };
    return r;
}
MATH_PREFIX Vec3 Round(const Vec3& a)
{
    Vec3 r = { roundf(a.x), roundf(a.y), roundf(a.z) };
    return r;
}
MATH_PREFIX Vec4 Round(const Vec4& a)
{
    Vec4 r = { roundf(a.x), roundf(a.y), roundf(a.z), roundf(a.w) };
    return r;
}
MATH_PREFIX float Sign(float value)
{
    return value < 0.0f ? -1.0f : 1.0f;
}
MATH_PREFIX i32 Sign(i32 value)
{
    return value < 0 ? -1 : 1;
}

template<typename T>
MATH_PREFIX bool IsNearZero(const T a)
{
    return a <= T(1.0e-12f);
}


[[nodiscard]] u32 PCG_Random(u64 state);
MATH_PREFIX u32 RandomU32(u64 state, u32 min, u32 max)
{
    u32 result = PCG_Random(state);
    //uint32 result = state;
    result ^= result << 13;
    result ^= result >> 17;
    result ^= result << 5;
    return (result % (max - min)) + min;
}

MATH_PREFIX float RandomFloat(const float min, const float max)
{
    return min + (max - min) * (rand() / float(RAND_MAX));
}

//Multiplication of two vectors without adding each dimension to get the dot product
template <typename T>
MATH_PREFIX gbVec2<T> HadamardProduct(const gbVec2<T>& a, const gbVec2<T>& b)
{
    return { a.x * b.x, a.y * b.y };
}
//Multiplication of two vectors without adding each dimension to get the dot product
template <typename T>
MATH_PREFIX gbVec3<T> HadamardProduct(const gbVec3<T>& a, const gbVec3<T>& b)
{
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}
//Multiplication of two vectors without adding each dimension to get the dot product
template <typename T>
MATH_PREFIX gbVec4<T> HadamardProduct(const gbVec4<T>& a, const gbVec4<T>& b)
{
    return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

template<typename T>
MATH_PREFIX gbVec3<T> Orthogonal(gbVec3<T> v)
{
    static_assert(sizeof(T) == 4);
    T x = fabsf(v.x);
    T y = fabsf(v.y);
    T z = fabsf(v.z);

    // Cross products work best when vectors are not colinear
    // so we should be selecting the 'other' vector that is most different
    // from the input vector.
    // Therefore if abs(x) is the smallest component then we are fine multiplying
    // with either oy or the ox vectors?
    gbVec3<T> ox = gbVec3<T>(1, 0, 0);
    gbVec3<T> oy = gbVec3<T>(0, 1, 0);
    gbVec3<T> oz = gbVec3<T>(0, 0, 1);
    gbVec3<T> other = x < y ? (x < z ? ox : oz) : (y < z ? oy : oz);
    other = CrossProduct(v, other);
    return other;
}

//rotation from u to v
template<typename T>
MATH_PREFIX gbQuat<T> RotationBetween(gbVec3<T> u, gbVec3<T> v)
{
    if (u == v || u == gbVec3<T>() || v == gbVec3<T>())
    {
        return gb_quat_identity<T>();
    }

    // It is important that the inputs are of equal length when
    // calculating the half-way vector.
    u = Normalize(u);
    v = Normalize(v);

    // Unfortunately, we have to check for when u == -v, as u + v
    // in this case will be (0, 0, 0), which cannot be normalized.
    if (u == -v)
    {
        // 180 degree rotation around any orthogonal vector
        u = Normalize(Orthogonal(u));

        gbQuat<T> q;
        q.xyz = u;
        q.w = 0;
        return q;
    }

    gbVec3<T> half = Normalize(u + v);
    gbVec3<T> cross = CrossProduct(u, half);

    gbQuat<T> result;
    result.xyz = cross;
    result.w = DotProduct(u, half);
    return result;
}

template<typename T>
MATH_PREFIX gbVec3<T> Vec3Forward()
{
    return { 1, 0, 0 };
}

template<typename T>
MATH_PREFIX gbQuat<T> RotationFromForward(const gbVec3<T>& v)
{
    return RotationBetween<T>(Vec3Forward<T>(), v);
}

template<typename T>
MATH_PREFIX gbQuat<T> OrientationForDirectionAndUp(const gbVec3<T>& forward, const gbVec3<T>& up)
{
    gbQuat<T> result = gb_quat_identity<T>();
    VALIDATE_V(PythagsInner<T>(forward)    > 0, result);
    VALIDATE_V(PythagsInner<T>(up)         > 0, result);
    if (Abs(DotProduct(forward, up)) > 0.95)
    {
        return RotationFromForward<T>(forward);
    }
    else
    {
        gbMat4<T> s = {};
        gbVec3<T> right = CrossProduct<T>(forward, up);
        s.col[0].xyz = forward;
        s.col[1].xyz = -right;
        s.col[2].xyz = CrossProduct<T>(forward, -right);
        gbQuat<T> mat_version = gb_quat_from_mat4<T>(s);
        return mat_version;
    }
}


template<typename T>
MATH_PREFIX gbVec3<T> GetScale(const gbMat4<T>& matrix)
{
    gbVec3<T> r;
    r.x = matrix.e[0];
    r.y = matrix.e[5];
    r.z = matrix.e[10];
    return r;
}

template<typename T>
MATH_PREFIX gbQuat<T> GetRotation(const gbMat4<T>& matrix) { return gb_quat_from_mat4<T>(matrix); }

template<typename T>
MATH_PREFIX gbVec3<T> GetPosition(const gbMat4<T>& matrix) { return matrix.col[3].xyz; }

template<typename T>
MATH_PREFIX gbVec3<T> GetForward(const gbMat4<T>& matrix) { return (matrix.x).xyz; }

template<typename T>
MATH_PREFIX gbVec3<T> GetUp(const gbMat4<T>& matrix)      { return matrix.z.xyz; }

template<typename T>
MATH_PREFIX gbVec3<T> GetLeft(const gbMat4<T>& matrix)    { return matrix.y.xyz; }

template<typename T = float>
constexpr [[nodiscard]] gbMat4<T> CreateModelMatrix(const gbVec3<T>& position, const gbQuat<T>& rotation, const gbVec3<T>& scale)
{
    const Mat4 t = gb_mat4_translate(position);
    const Mat4 r = gb_mat4_from_quat(rotation);
    const Mat4 s = gb_mat4_scale(scale);
    Mat4 rr = t * r * s;
    return rr;
}

template<typename T = float>
constexpr [[nodiscard]] gbMat4<T> CreateModelMatrix(const STransform& t)
{
    return CreateModelMatrix(t.pos, t.rot, t.scale);
}

template<typename T>
[[nodiscard]] gbMat3<T> Adjugate(const gbMat4<T>& m)
{
    return gbMat3(CrossProduct(m.y.xyz, m.z.xyz),
                  CrossProduct(m.z.xyz, m.x.xyz),
                  CrossProduct(m.x.xyz, m.y.xyz));
}

template<typename T>
inline STransform TransformFromMatrix(const gbMat4<T>& mat)
{
    Vec3 translation = mat.col[3].xyz;
    Vec3 scale = {
        Length(GetForward(mat)),
        Length(GetLeft(mat)),
        Length(GetUp(mat)),
    };

    Mat3 rot = ToMat3(mat);
    rot.col[0].x /= scale.x;
    rot.col[0].y /= scale.x;
    rot.col[0].z /= scale.x;
    rot.col[1].x /= scale.y;
    rot.col[1].y /= scale.y;
    rot.col[1].z /= scale.y;
    rot.col[2].x /= scale.z;
    rot.col[2].y /= scale.z;
    rot.col[2].z /= scale.z;

    STransform result;
    result.pos = translation;
    result.rot = gb_quat_from_mat3(rot);
    result.scale = scale;
    return result;
}


struct Frustum {
    Vec4 e[6];
};


struct Sphere{
    Vec3 center;
    float radius;
};

struct Capsule {
    Vec3 center;
    float height;
    float radius;
    Quat rotation;
};

struct AABB {
    Vec3 min = {};
    Vec3 max = {};

    [[nodiscard]] Vec3 GetLengths() const
    {
        Vec3 result = {};
        result.x = Abs(max.x - min.x);
        result.y = Abs(max.y - min.y);
        result.z = Abs(max.z - min.z);
        return result;
    }

    [[nodiscard]] Vec3 Center() const
    {
        Vec3 result = {};
        result = min + ((max - min) / 2.0f);
        return result;
    }
    void MoveCenter(const Vec3& new_position)
    {
        Vec3 lengths = GetLengths();
        min = new_position - (lengths / 2.0f);
        max = new_position + (lengths / 2.0f);
    }
};

struct OBB {
    Vec3 center = {};
    Vec3 half_width = {};
    Quat rotation = gb_quat_identity();
};




union Triangle {
    struct { Vec3 a, b, c; };
    Vec3 e[3];

    Vec3 Normal() const
    {
        Vec3 r = Normalize(CrossProduct(b - a, c - a));
        return r;
    }
    Vec3 Average() const
    {
        Vec3 r = (a + b + c) / 3.0f;
        return r;
    }
    Vec3 MidPoint() const
    {
        Vec3 min = Vec3(+inf);
        Vec3 max = Vec3(-inf);
        for (i32 i = 0; i < (sizeof(e) / sizeof(e[0])); i++)
        {
            min = Min(min, e[i]);
            max = Max(max, e[i]);
        }

        Vec3 r = (max - min) / 2.0f;
        return r;
    }
    void Scale(const Vec3& scale)
    {
        const Vec3 center = MidPoint();
        for (i32 i = 0; i < (sizeof(e) / sizeof(e[0])); i++)
        {
            e[i] = center + HadamardProduct((e[i] - center), scale);
        }
    }
};

Frustum ComputeFrustum(const Mat4& mvProj);
bool IsBoxInFrustum(const Frustum& f, float* bmin, float* bmax);
bool SphereVsFrustum(const Sphere& sphere, const Frustum& frustum, bool ignore_near_z = false);
i32 ManhattanDistance(Vec3I a, Vec3I b);


//Vec3 ClosestPointOnLineSegment(const Vec3& A, const Vec3& B, const Vec3& Point);

MATH_PREFIX int QuickSortComparisonFunction(const void* a, const void* b)
{
    return  *(i32*)b - *(i32*)a;
}
MATH_PREFIX int QuickSortComparisonFunction_uint64_t(const void* a, const void* b)
{
    return  int(*(uint64_t*)b - *(uint64_t*)a);
}
template<typename T>
MATH_PREFIX int QuickSortComparisonFunction(const void* a, const void* b)
{
    return  int(*(T*)b - *(T*)a);
}
void QuickSort(u8* data, const i32 length, const i32 itemSize, i32 (*compare)(const void* a, const void* b));