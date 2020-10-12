/*
Copyright (c) 2003-2009 Erwin Coumans  http://bullet.googlecode.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If
you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not
required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef BT_SCALAR_H
#define BT_SCALAR_H

#include <float.h>
#include <math.h>
#include <stdlib.h> //size_t for MSVC 6.0
#include <stdint.h>

#if defined(DEBUG) || defined(_DEBUG)
#    define BT_DEBUG
#endif

#include <assert.h>
#ifdef BT_DEBUG
#    define btAssert assert
#else
#    define btAssert(x)
#endif
// btFullAssert is optional, slows down a lot
#define btFullAssert(x)

#define btLikely(_c) _c
#define btUnlikely(_c) _c

/// The btScalar type abstracts floating point numbers, to easily switch between double and single floating point
/// precision.
typedef float btScalar;
#define BT_LARGE_FLOAT 1e18f

inline btScalar btSqrt(btScalar y)
{
    return sqrtf(y);
}

inline btScalar btFabs(btScalar x)
{
    return fabsf(x);
}
inline btScalar btCos(btScalar x)
{
    return cosf(x);
}
inline btScalar btSin(btScalar x)
{
    return sinf(x);
}
inline btScalar btTan(btScalar x)
{
    return tanf(x);
}
inline btScalar btAcos(btScalar x)
{
    if (x < btScalar(-1))
        x = btScalar(-1);
    if (x > btScalar(1))
        x = btScalar(1);
    return acosf(x);
}
inline btScalar btAsin(btScalar x)
{
    if (x < btScalar(-1))
        x = btScalar(-1);
    if (x > btScalar(1))
        x = btScalar(1);
    return asinf(x);
}
inline btScalar btAtan(btScalar x)
{
    return atanf(x);
}
inline btScalar btAtan2(btScalar x, btScalar y)
{
    return atan2f(x, y);
}
inline btScalar btExp(btScalar x)
{
    return expf(x);
}
inline btScalar btLog(btScalar x)
{
    return logf(x);
}
inline btScalar btPow(btScalar x, btScalar y)
{
    return powf(x, y);
}
inline btScalar btFmod(btScalar x, btScalar y)
{
    return fmodf(x, y);
}


#define SIMD_2_PI btScalar(6.283185307179586232)
#define SIMD_PI (SIMD_2_PI * btScalar(0.5))
#define SIMD_HALF_PI (SIMD_2_PI * btScalar(0.25))
#define SIMD_RADS_PER_DEG (SIMD_2_PI / btScalar(360.0))
#define SIMD_DEGS_PER_RAD (btScalar(360.0) / SIMD_2_PI)
#define SIMDSQRT12 btScalar(0.7071067811865475244008443621048490)

#define btRecipSqrt(x) ((btScalar)(btScalar(1.0) / btSqrt(btScalar(x)))) /* reciprocal square root */

#define SIMD_INFINITY FLT_MAX

inline btScalar btAtan2Fast(btScalar y, btScalar x)
{
    btScalar coeff_1 = SIMD_PI / 4.0f;
    btScalar coeff_2 = 3.0f * coeff_1;
    btScalar abs_y = btFabs(y);
    btScalar angle;
    if (x >= 0.0f)
    {
        btScalar r = (x - abs_y) / (x + abs_y);
        angle = coeff_1 - coeff_1 * r;
    }
    else
    {
        btScalar r = (x + abs_y) / (abs_y - x);
        angle = coeff_2 - coeff_1 * r;
    }
    return (y < 0.0f) ? -angle : angle;
}

inline bool btFuzzyZero(btScalar x)
{
    return btFabs(x) < FLT_EPSILON;
}

inline bool btEqual(btScalar a, btScalar eps)
{
    return (((a) <= eps) && !((a) < -eps));
}
inline bool btGreaterEqual(btScalar a, btScalar eps)
{
    return (!((a) <= eps));
}

inline int32_t btIsNegative(btScalar x)
{
    return x < btScalar(0.0) ? 1 : 0;
}

inline btScalar btRadians(btScalar x)
{
    return x * SIMD_RADS_PER_DEG;
}
inline btScalar btDegrees(btScalar x)
{
    return x * SIMD_DEGS_PER_RAD;
}

#define BT_DECLARE_HANDLE(name)                                                                                        \
    typedef struct name##__                                                                                            \
    {                                                                                                                  \
        int32_t unused;                                                                                                \
    } * name

#ifndef btFsel
inline btScalar btFsel(btScalar a, btScalar b, btScalar c)
{
    return a >= 0 ? b : c;
}
#endif
#define btFsels(a, b, c) (btScalar) btFsel(a, b, c)

inline bool btMachineIsLittleEndian()
{
    long int i = 1;
    const char* p = (const char*)&i;
    if (p[0] == 1) // Lowest address contains the least significant byte
        return true;
    else
        return false;
}

/// btSelect avoids branches, which makes performance much better for consoles like Playstation 3 and XBox 360
/// Thanks Phil Knight. See also http://www.cellperformance.com/articles/2006/04/more_techniques_for_eliminatin_1.html
inline unsigned btSelect(unsigned condition, unsigned valueIfConditionNonZero, unsigned valueIfConditionZero)
{
    // Set testNz to 0xFFFFFFFF if condition is nonzero, 0x00000000 if condition is zero
    // Rely on positive value or'ed with its negative having sign bit on
    // and zero value or'ed with its negative (which is still zero) having sign bit off
    // Use arithmetic shift right, shifting the sign bit through all 32 bits
    unsigned testNz = (unsigned)(((int32_t)condition | -(int32_t)condition) >> 31);
    unsigned testEqz = ~testNz;
    return ((valueIfConditionNonZero & testNz) | (valueIfConditionZero & testEqz));
}
inline int32_t btSelect(unsigned condition, int32_t valueIfConditionNonZero, int32_t valueIfConditionZero)
{
    unsigned testNz = (unsigned)(((int32_t)condition | -(int32_t)condition) >> 31);
    unsigned testEqz = ~testNz;
    return static_cast<int32_t>((valueIfConditionNonZero & testNz) | (valueIfConditionZero & testEqz));
}
inline float btSelect(unsigned condition, float valueIfConditionNonZero, float valueIfConditionZero)
{
    return (condition != 0) ? valueIfConditionNonZero : valueIfConditionZero;
}

template <typename T>
inline void btSwap(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

// PCK: endian swapping functions
inline unsigned btSwapEndian(unsigned val)
{
    return (((val & 0xff000000) >> 24) | ((val & 0x00ff0000) >> 8) | ((val & 0x0000ff00) << 8) |
            ((val & 0x000000ff) << 24));
}

inline unsigned short btSwapEndian(unsigned short val)
{
    return static_cast<unsigned short>(((val & 0xff00) >> 8) | ((val & 0x00ff) << 8));
}

inline unsigned btSwapEndian(int32_t val)
{
    return btSwapEndian((unsigned)val);
}

inline unsigned short btSwapEndian(short val)
{
    return btSwapEndian((unsigned short)val);
}

/// btSwapFloat uses using char pointers to swap the endianness
////btSwapFloat/btSwapDouble will NOT return a float, because the machine might 'correct' invalid floating point values
/// Not all values of sign/exponent/mantissa are valid floating point numbers according to IEEE 754.
/// When a floating point unit is faced with an invalid value, it may actually change the value, or worse, throw an
/// exception. In most systems, running user mode code, you wouldn't get an exception, but instead the
/// hardware/os/runtime will 'fix' the number for you. so instead of returning a float/double, we return integer/long
/// long integer
inline uint32_t btSwapEndianFloat(float d)
{
    uint32_t a = 0;
    unsigned char* dst = (unsigned char*)&a;
    unsigned char* src = (unsigned char*)&d;

    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
    return a;
}

// unswap using char pointers
inline float btUnswapEndianFloat(uint32_t a)
{
    float d = 0.0f;
    unsigned char* src = (unsigned char*)&a;
    unsigned char* dst = (unsigned char*)&d;

    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];

    return d;
}

// swap using char pointers
inline void btSwapEndianDouble(double d, unsigned char* dst)
{
    unsigned char* src = (unsigned char*)&d;

    dst[0] = src[7];
    dst[1] = src[6];
    dst[2] = src[5];
    dst[3] = src[4];
    dst[4] = src[3];
    dst[5] = src[2];
    dst[6] = src[1];
    dst[7] = src[0];
}

// unswap using char pointers
inline double btUnswapEndianDouble(const unsigned char* src)
{
    double d = 0.0;
    unsigned char* dst = (unsigned char*)&d;

    dst[0] = src[7];
    dst[1] = src[6];
    dst[2] = src[5];
    dst[3] = src[4];
    dst[4] = src[3];
    dst[5] = src[2];
    dst[6] = src[1];
    dst[7] = src[0];

    return d;
}

// returns normalized value in range [-SIMD_PI, SIMD_PI]
inline btScalar btNormalizeAngle(btScalar angleInRadians)
{
    angleInRadians = btFmod(angleInRadians, SIMD_2_PI);
    if (angleInRadians < -SIMD_PI)
    {
        return angleInRadians + SIMD_2_PI;
    }
    else if (angleInRadians > SIMD_PI)
    {
        return angleInRadians - SIMD_2_PI;
    }
    else
    {
        return angleInRadians;
    }
}

/// rudimentary class to provide type info
struct btTypedObject
{
    btTypedObject(int32_t objectType) : m_objectType(objectType)
    {
    }
    int32_t m_objectType;
    inline int32_t getObjectType() const
    {
        return m_objectType;
    }
};
#endif // BT_SCALAR_H
