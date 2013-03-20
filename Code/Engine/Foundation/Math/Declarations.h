#pragma once

#include <Foundation/Basics.h>

#define ezMath_e 2.71828182845904f
#define ezMath_Pi 3.1415926535897932384626433832795f  ///< Pi
#define ezMath_DegToRad (ezMath_Pi / 180.0f)          ///< Constant to convert from degree to rad
#define ezMath_RadToDeg (180.f / ezMath_Pi)           ///< Constant to convert from rad to degree
#define ezMath_FloatMax_Pos  3.402823465e+38F         ///< The largest positive number that floats can represent.
#define ezMath_FloatMax_Neg -ezMath_FloatMax_Pos      ///< The largest negative number that floats can represent.

#define ezMath_FloatEpsilon     1.192092896e-07F      ///< Smallest representable value for floats
#define ezMath_SmallEpsilon     0.000001f
#define ezMath_DefaultEpsilon   0.00001f
#define ezMath_LargeEpsilon     0.0001f
#define ezMath_HugeEpsilon      0.001f

/// \brief Simple helper union to store ints and floats to modify their bit patterns.
union ezIntFloatUnion
{
  ezUInt32 i;
  float f;
};

/// \brief Enum to describe which memory layout is used to store a matrix in a float array.
struct ezMatrixLayout
{
  enum Enum
  {
    RowMajor,     ///< The matrix is store in row-major format.
    ColumnMajor   ///< The matrix is store in column-major format.
  };
};

struct ezProjectionDepthRange
{
  enum Enum
  {
    MinusOneToOne,  
    ZeroToOne,      
  };
};


// forward declarations
class ezVec2;
class ezVec3;
class ezVec4;
class ezMat3;
class ezMat4;
class ezPlane;
class ezQuat;
class ezBoundingBox;
class ezBoundingSphere;
