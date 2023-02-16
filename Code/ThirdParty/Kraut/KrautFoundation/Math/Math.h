#ifndef AE_FOUNDATION_MATH_MATH_H
#define AE_FOUNDATION_MATH_MATH_H

#include "Declarations.h"
#include "../Basics/Checks.h"

namespace AE_NS_FOUNDATION
{
  #define aeMath_PI 3.1415926535897932384626433832795f  //!< PI
  #define aeMath_DegToRad (aeMath_PI / 180.0f)          //!< Constant to convert from degree to rad
  #define aeMath_RadToDeg (180.f / aeMath_PI)           //!< Constant to convert from rad to degree
  #define aeMath_Epsilon 1.192092896e-07F               //!< Smallest representable value for floats
  #define aeMath_LargeEpsilon 0.000001f                 //!< Some arbirary but more useful epsilon value
  #define aeMath_HugeEpsilon 0.0001f                    //!< Some even larger epsilon value for when aeMath_LargeEpsilon is too small
  #define aeMath_GiganticEpsilon 0.01f                  //!< Some even larger epsilon value for when even aeMath_HugeEpsilon is too small
  #define aeMath_FloatMax_Pos  3.402823465e+38F         //!< The largest positive number that floats can represent.
  #define aeMath_FloatMax_Neg -aeMath_FloatMax_Pos      //!< The smallest negative number that floats can represent.

  //! This class provides much common math-functionality as static functions.
  struct AE_FOUNDATION_DLL aeMath
  {
    static float PI ()              { return aeMath_PI;             }
    static float Epsilon ()         { return aeMath_Epsilon;        }
    static float LargeEpsilon ()    { return aeMath_LargeEpsilon;   }
    static float HugeEpsilon ()     { return aeMath_HugeEpsilon;    }
    static float GiganticEpsilon () { return aeMath_GiganticEpsilon;}
    static float FloatMax_Pos ()    { return aeMath_FloatMax_Pos;   }
    static float FloatMax_Neg ()    { return aeMath_FloatMax_Neg;   }
    static float DegToRad (float f) { return aeMath_DegToRad * f;   }
    static float RadToDeg (float f) { return aeMath_RadToDeg * f;   }
    //! Returns the a float that represents '+Infinity'.
    static float Infinity ();

    //!  Takes an angle in degree, returns its sine
    static float SinDeg (float f);
    //!  Takes an angle in degree, returns its cosine
    static float CosDeg (float f);
    //!  Takes an angle in radians, returns its sine
    static float SinRad (float f);
    //!  Takes an angle in radians, returns its cosine
    static float CosRad (float f);
    //!  Takes an angle in degree, returns its tangens
    static float TanDeg (float f);
    //!  Takes an angle in radians, returns its tangens
    static float TanRad (float f);

    //!  Returns the arcus sinus of f, in degree
    static float ASinDeg  (float f);
    //!  Returns the arcus cosinus of f, in degree
    static float ACosDeg  (float f);
    //!  Returns the arcus sinus of f, in radians
    static float ASinRad  (float f);
    //!  Returns the arcus cosinus of f, in radians
    static float ACosRad  (float f);
    //!  Returns the arcus tangens of f, in degree
    static float ATanDeg  (float f);
    //!  Returns the arcus tangens of f, in radians
    static float ATanRad  (float f);
    //!  Returns the atan2 of x and y, in degree
    static float ATan2Deg (float x, float y);
    //!  Returns the atan2 of x and y, in radians
    static float ATan2Rad (float x, float y);

    //!  Returns e^f
    static float Exp   (float f);
    //!  Returns the logarithmus naturalis of f
    static float Ln    (float f);
    //!  Returns log (f), to the base 2
    static float Log2  (float f);
    //!  Returns log (f), to the base 10
    static float Log10 (float f);
    //!  Returns log (f), to the base fBase
    static float Log (float fBase, float f);
    //!  Returns 2^f
    static float Pow2  (float f);
    //!  Returns base^exp
    static float Pow (float base, float exp);
    //!  Returns 2^f
    static int Pow2  (int i);
    //!  Returns base^exp
    static int Pow (int base, int exp);
    //!  Returns f * f
    template<class T>
    inline static T Square (T f) {return (f * f);}
    //!  Returns the square root of f
    static float Sqrt (float f);
    //!  Returns the n-th root of f.
    static float Root (float f, float NthRoot);

    //!  Returns the sign of f (ie: -1, 1 or 0)
    template<class T>
    static T Sign (T f) {return (f < 0 ? T (-1) : f > 0 ? T (1) : 0);}
    //!  Returns the absolute value of f
    template<class T>
    static T Abs (T f) {return (f < 0 ? -f : f);}
    //!  Returns the smaller value, f1 or f2
    template<class T>
    static T Min (T f1, T f2) {return (f1 < f2 ? f1 : f2);}
    //!  Returns the smaller value, f1 or f2 or f3
    template<class T>
    static T Min (T f1, T f2, T f3) {return Min (Min (f1, f2), f3);}
    //!  Returns the smaller value, f1 or f2 or f3 or f4
    template<class T>
    static T Min (T f1, T f2, T f3, T f4) {return Min (Min (f1, f2), Min (f3, f4));}
    //!  Returns the greater value, f1 or f2
    template<class T>
    static T Max (T f1, T f2) {return (f1 > f2 ? f1 : f2);}
    //!  Returns the smaller value, f1 or f2 or f3
    template<class T>
    static T Max (T f1, T f2, T f3) {return Max (Max (f1, f2), f3);}
    //!  Returns the smaller value, f1 or f2 or f3 or f4
    template<class T>
    static T Max (T f1, T f2, T f3, T f4) {return Max (Max (f1, f2), Max (f3, f4));}

    //!  Clamps "value" to the range [min; max]. Returns "value", if it is inside the range already
    template<class T>
    static T Clamp (T value, T min_val, T max_val) {if (value < min_val) return (min_val); 
    if (value > max_val) return (max_val);	return (value);	}

    //!  Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
    static float Floor (float f);
    //!  Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
    static float Ceil  (float f);
    //! Returns a multiple of fMultiple that is smaller than f.
    static float Floor (float f, float fMultiple);
    //! Returns a multiple of fMultiple that is larger than f.
    static float Ceil (float f, float fMultiple);
    //! Returns a multiple of uiMultiple that is smaller than i.
    static aeInt32 Floor (aeInt32 i, aeUInt32 uiMultiple);
    //! Returns a multiple of uiMultiple that is larger than i.
    static aeInt32 Ceil (aeInt32 i, aeUInt32 uiMultiple);

    //!  Returns the integer-part of f (removes the fraction).
    static float Trunc (float f);
    //!  Rounds f to the next integer. If f is positive 0.5 is rounded UP (ie. to 1), if f is negative, -0.5 is rounded DOWN (ie. to -1).
    static float Round (float f);
    //!  Rounds f to the closest multiple of fRoundTo.
    static float Round (float f, float fRoundTo);
    //!  Returns the fraction-part of f.
    static float Fraction (float f);
    //!  Converts f into an integer.
    static aeInt32 FloatToInt (float f);

    //!  Returns "value mod div" for floats.
    static float Mod (float f, float div);
    //!  Returns 1 / f
    static float Invert (float f);

    //!  Returns true, if i is an odd number
    static bool IsOdd (aeInt32 i);
    //!  Returns true, if i is an even number
    static bool IsEven (aeInt32 i);
    //!  Swaps the values in the two variables f1 and f2
    template<class T>
    static void Swap (T& f1, T& f2) {T temp = f1; f1 = f2; f2 = temp;}


    //!  Returns the linear interpoltion of f1 and f2. factor is a value between 0 and 1.
    template<class T>
    static T Lerp (T f1, T f2, float factor) 	
    {
      AE_CHECK_DEV ((factor >= 0.0f) && (factor <= 1.0f), "aeMath::lerp: factor %.2f is not in the range [0; 1]", factor);
      return ((T) (f1 * (1.0f - factor) + f2 * factor));
    }

    //!  Returns 0, if value < edge, and 1, if value >= edge.
    template<class T>
    static T Step (T value, T edge) {return (value >= edge ? T (1) : T (0));}

    //!  Returns 0, if value is <= edge1, 1 if value >= edge2 and the hermite interpolation in between
    static float SmoothStep (float value, float edge1, float edge2);

    //!  Returns true, if there exists some x with base^x == value
    static bool IsPowerOf (aeInt32 value, aeInt32 base);
    //!  Returns true, if there exists some x with 2^x == value
    static bool IsPowerOf2 (aeInt32 value);
    //!  Returns the next power-of-two that is <= value
    static aeInt32 PowerOfTwo_Floor (aeUInt32 value);
    //!  Returns the next power-of-two that is >= value
    static aeInt32 PowerOfTwo_Ceil (aeUInt32 value);

    //!  Checks, whether fValue is in the range [fDesired - fMaxImprecision; fDesired + fMaxImprecision].
    static bool IsFloatEqual (float lhs, float rhs, float fEpsilon = aeMath_LargeEpsilon);

    //!  Returns true if f is NaN.
    static bool IsNaN (float f);
    //! Returns whether f is a valid finite float (e.g. not NaN and not +/-Inifnity).
    static bool IsFinite (float f);

    //! Checks if there is an intersection between the given ray and the polygon. If so returns true and stores the intersection time (multiple of vRayDirNorm) and point in the out-parameters.
    static bool GetRayPolygonIntersection (const aeVec3* pVertices, aeUInt32 uiVertices, const aeVec3& vRayStart, const aeVec3& vRayDirNorm, float& out_fIntersectionTime, aeVec3& out_vIntersection);
    //! Checks if a given point lies inside a convex polygon.
    static bool IsPointInPolygon (const aeVec3* pVertices, aeUInt32 uiVertices, const aeVec3& vPoint);
  };


  // ***** Default Comparison Functors *****

  template<class KEY>
  class aeCompareLess
  {
  public:
    bool operator() (const KEY& key1, const KEY& key2)
    {
      return (key1 < key2);
    }
  };

  template<class KEY>
  class aeCompareGreater
  {
  public:
    bool operator() (const KEY& key1, const KEY& key2)
    {
      return (key1 > key2);
    }
  };
}

#include "Inline/MathFunctions.inl"

#endif


