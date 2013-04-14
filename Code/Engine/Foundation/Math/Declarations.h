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
///
/// All ezMatX classes use column-major format internally. That means they contain one array
/// of, e.g. 16 elements, and the first elements represent the first column, then the second column, etc.
/// So the data is stored column by column and is thus column-major.
/// Some other libraries, such as OpenGL or DirectX require data represented either in column-major
/// or row-major format. ezMatrixLayout allows to retrieve the data from an ezMatX class in the proper format,
/// and it also allows to pass matrix data as an array back in the ezMatX class, and have it converted properly.
/// That means, if you need to pass the content of an ezMatX to a function that requires the data in row-major
/// format, you specify that you want to convert the matrix to ezMatrixLayout::RowMajor format and you will get
/// the data properly transposed. If a function requires data in column-major format, you specify
/// ezMatrixLayout::ColumnMajor and you get it in column-major format (which is simply a memcpy).
struct ezMatrixLayout
{
  enum Enum
  {
    RowMajor,     ///< The matrix is store in row-major format.
    ColumnMajor   ///< The matrix is store in column-major format.
  };
};

/// \brief This enum describes for which depth range a projection matrix is constructed.
///
/// Different Rendering APIs use different depth ranges.
/// E.g. OpenGL uses -1 for the near plane and +1 for the far plane.
/// DirectX uses 0 for the near plane and 1 for the far plane.
struct ezProjectionDepthRange
{
  enum Enum
  {
    MinusOneToOne,  ///< Near plane at -1, far plane at +1
    ZeroToOne,      ///< Near plane at 0, far plane at 1
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
