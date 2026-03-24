/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         Geometry.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Geometric classes: Rect, Vec3, Point, Matrix, Plane
*/

#pragma once

#include "Types.h"

// Minimal Unreal include for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

struct Rect;
struct Insets;
struct Matrix;
struct Quaternion;
struct Plane;

inline constexpr double DEGREES = (PI / 180.0);

// +--------------------------------------------------------------------+

struct Rect
{
    static const char* TYPENAME() { return "Rect"; }

    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int ix, int iy, int iw, int ih) : x(ix), y(iy), w(iw), h(ih) {}

    int   operator==(const Rect& r) const { return x == r.x && y == r.y && w == r.w && h == r.h; }
    int   operator!=(const Rect& r) const { return x != r.x || y != r.y || w != r.w || h != r.h; }

    void  Inflate(int dw, int dh);
    void  Deflate(int dw, int dh);
    void  Inset(int left, int right, int top, int bottom);
    int   Contains(int x, int y) const;

    int x, y, w, h;
};

// +--------------------------------------------------------------------+

struct Insets
{
    Insets() : left(0), right(0), top(0), bottom(0) {}
    Insets(WORD l, WORD r, WORD t, WORD b) : left(l), right(r), top(t), bottom(b) {}

    WORD  left;
    WORD  right;
    WORD  top;
    WORD  bottom;
};

// +--------------------------------------------------------------------+

struct Matrix
{
    static const char* TYPENAME() { return "Matrix"; }

    Matrix();
    Matrix(const Matrix& m);
    Matrix(const FVector& vrt, const FVector& vup, const FVector& vpn);

    Matrix& operator =  (const Matrix& m);
    Matrix& operator *= (const Matrix& m);

    double  operator() (int i, int j) const { return elem[i][j]; }
    double& operator() (int i, int j) { return elem[i][j]; }

    void Identity();
    void Transpose();
    void Rotate(double roll, double pitch, double yaw);
    void Roll(double roll);
    void Pitch(double pitch);
    void Yaw(double yaw);
    void ComputeEulerAngles(double& roll, double& pitch, double& yaw) const;

    double Cofactor(int i, int j) const;
    void   Invert();

    Matrix Inverse() const {
        Matrix result(*this);
        result.Invert();
        return result;
    }

    Matrix  operator*(const Matrix& m) const;
    FVector operator*(const FVector& p) const;

    double elem[3][3];

private:
    Matrix(int no_init) {}
};

// +--------------------------------------------------------------------+

struct Vec2
{
    static const char* TYPENAME() { return "Vec2"; }

    Vec2() {}
    Vec2(int    ix, int    iy) : x((float)ix), y((float)iy) {}
    Vec2(float  ix, float  iy) : x(ix), y(iy) {}
    Vec2(double ix, double iy) : x((float)ix), y((float)iy) {}

    operator void* ()            const { return (void*)(x || y); }
    int    operator==(const Vec2& p) const { return x == p.x && y == p.y; }
    int    operator!=(const Vec2& p) const { return x != p.x || y != p.y; }
    Vec2   operator+ (const Vec2& p) const { return Vec2(x + p.x, y + p.y); }
    Vec2   operator- (const Vec2& p) const { return Vec2(x - p.x, y - p.y); }
    Vec2   operator- ()              const { return Vec2(-x, -y); }
    Vec2   operator* (float s)       const { return Vec2(x * s, y * s); }
    Vec2   operator/ (float s)       const { return Vec2(x / s, y / s); }
    float  operator* (const Vec2& p) const { return (x * p.x + y * p.y); }

    Vec2& operator= (const Vec2& p) { x = p.x; y = p.y; return *this; }
    Vec2& operator+=(const Vec2& p) { x += p.x; y += p.y; return *this; }
    Vec2& operator-=(const Vec2& p) { x -= p.x; y -= p.y; return *this; }
    Vec2& operator*=(float  s) { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(float  s) { x /= s;   y /= s;   return *this; }

    float  length()           const { return (float)sqrt(x * x + y * y); }
    float  Normalize();
    float  dot(const Vec2& p) const { return (x * p.x + y * p.y); }
    Vec2   normal()           const { return Vec2(-y, x); }

    float x, y;
};

// +--------------------------------------------------------------------+
// Vec3/Point are now Unreal FVector (float).

using Vec3 = FVector;
using Point = FVector;

// ClosestApproachTime now uses FVector overload:
double ClosestApproachTime(const FVector& loc1, const FVector& vel1,
    const FVector& loc2, const FVector& vel2);

// +--------------------------------------------------------------------+

struct Quaternion
{
    static const char* TYPENAME() { return "Quaternion"; }

    Quaternion() : x(0), y(0), z(0), w(0) {}
    Quaternion(double ix,
        double iy,
        double iz,
        double iw) : x(ix), y(iy), z(iz), w(iw) {
    }
    Quaternion(const Quaternion& q) : x(q.x), y(q.y), z(q.z), w(q.w) {}

    int   operator==(const Quaternion& q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
    int   operator!=(const Quaternion& q) const { return x != q.x || y != q.y || z != q.z || w != q.w; }

    Quaternion operator+ (const Quaternion& q) const { return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w); }
    Quaternion operator- (const Quaternion& q) const { return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w); }
    Quaternion operator- ()                  const { return Quaternion(-x, -y, -z, -w); }
    Quaternion operator* (double s)          const { return Quaternion(x * s, y * s, z * s, w * s); }
    Quaternion operator/ (double s)          const { return Quaternion(x / s, y / s, z / s, w / s); }

    Quaternion& operator= (const Quaternion& q) { x = q.x; y = q.y; z = q.z; w = q.w; return *this; }
    Quaternion& operator+=(const Quaternion& q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
    Quaternion& operator-=(const Quaternion& q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
    Quaternion& operator*=(double s) { x *= s;   y *= s;   z *= s;   w *= s;   return *this; }
    Quaternion& operator/=(double s) { x /= s;   y /= s;   z /= s;   w /= s;   return *this; }

    double length()    const { return sqrt(x * x + y * y + z * z + w * w); }
    double Normalize();

    double x, y, z, w;
};

// +--------------------------------------------------------------------+

struct Plane
{
    static const char* TYPENAME() { return "Plane"; }

    Plane();
    Plane(const FVector& p0, const FVector& p1, const FVector& p2);

    void Rotate(const FVector& v0, const Matrix& m);
    void Translate(const FVector& v0);

    float    distance = 0.0f;
    FVector  normal = FVector::ZeroVector;
};

// +--------------------------------------------------------------------+

double DotProduct(const FVector& a, const FVector& b);
void   CrossProduct(const FVector& a, const FVector& b, FVector& out);
void   MConcat(double in1[3][3], double in2[3][3], double out[3][3]);

// +--------------------------------------------------------------------+

int lines_intersect(
    /* 1st line segment */ double x1, double y1, double x2, double y2,
    /* 2nd line segment */ double x3, double y3, double x4, double y4,
    /* intersect point  */ double& x, double& y);

// +--------------------------------------------------------------------+
