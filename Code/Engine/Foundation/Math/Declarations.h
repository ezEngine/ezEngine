#pragma once

/// \file

#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
#  define EZ_NAN_ASSERT(obj) (obj)->AssertNotNaN();
#else
#  define EZ_NAN_ASSERT(obj)
#endif

/// \brief Simple helper union to store ints and floats to modify their bit patterns.
union ezIntFloatUnion {
  constexpr ezIntFloatUnion(float init)
    : f(init)
  {
  }

  constexpr ezIntFloatUnion(ezUInt32 init)
    : i(init)
  {
  }

  ezUInt32 i;
  float f;
};

/// \brief Simple helper union to store ints and doubles to modify their bit patterns.
union ezInt64DoubleUnion {

  constexpr ezInt64DoubleUnion(double init)
    : f(init)
  {
  }
  constexpr ezInt64DoubleUnion(ezUInt64 init)
    : i(init)
  {
  }

  ezUInt64 i;
  double f;
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
    RowMajor,   ///< The matrix is stored in row-major format.
    ColumnMajor ///< The matrix is stored in column-major format.
  };
};

/// \brief Describes for which depth range a projection matrix is constructed.
///
/// Different Rendering APIs use different depth ranges.
/// E.g. OpenGL uses -1 for the near plane and +1 for the far plane.
/// DirectX uses 0 for the near plane and 1 for the far plane.
struct ezClipSpaceDepthRange
{
  enum Enum
  {
    MinusOneToOne, ///< Near plane at -1, far plane at +1
    ZeroToOne,     ///< Near plane at 0, far plane at 1
  };

  /// \brief Holds the default value for the projection depth range on each platform.
  /// This can be overridden by renderers to ensure the proper range is used when they become active.
  /// On Windows/D3D this is initialized with 'ZeroToOne' by default on all other platforms/OpenGL it is initialized with 'MinusOneToOne' by default.
  EZ_FOUNDATION_DLL static Enum Default;
};

/// \brief Specifies whether a projection matrix should flip the result along the Y axis or not.
///
/// Mostly needed to compensate for differing Y texture coordinate conventions. Ie. on some platforms
/// the Y texture coordinate origin is at the lower left and on others on the upper left. To prevent having
/// to modify content to compensate, instead textures are simply flipped along Y on texture load.
/// The same has to be done for all render targets, ie. content has to be rendered upside-down.
///
/// Use ezClipSpaceYMode::RenderToTextureDefault when rendering to a texture, to always get the correct
/// projection matrix.
struct ezClipSpaceYMode
{
  enum Enum
  {
    Regular, ///< Creates a regular projection matrix
    Flipped, ///< Creates a projection matrix that flips the image on its head. On platforms with different Y texture coordinate
             ///< conventions, this can be used to compensate, by rendering images flipped to render targets.
  };

  /// \brief Holds the platform default value for the clip space Y mode when rendering to a texture.
  /// This can be overridden by renderers to ensure the proper mode is used when they become active.
  /// On Windows/D3D this is initialized with 'Regular' by default on all other platforms/OpenGL it is initialized with 'Flipped' by default.
  EZ_FOUNDATION_DLL static Enum RenderToTextureDefault;
};

/// \brief For selecting a left-handed or right-handed convention
struct ezHandedness
{
  enum Enum
  {
    LeftHanded,
    RightHanded,
  };

  /// \brief Holds the default handedness value to use. ez uses 'LeftHanded' by default.
  EZ_FOUNDATION_DLL static Enum Default /*= ezHandedness::LeftHanded*/;
};

// forward declarations
template <typename Type>
class ezVec2Template;

using ezVec2 = ezVec2Template<float>;
using ezVec2d = ezVec2Template<double>;
using ezVec2I32 = ezVec2Template<ezInt32>;
using ezVec2U32 = ezVec2Template<ezUInt32>;

template <typename Type>
struct ezVec3Template;

using ezVec3 = ezVec3Template<float>;
using ezVec3d = ezVec3Template<double>;
using ezVec3I32 = ezVec3Template<ezInt32>;
using ezVec3U32 = ezVec3Template<ezUInt32>;

template <typename Type>
class ezVec4Template;

using ezVec4 = ezVec4Template<float>;
using ezVec4d = ezVec4Template<double>;
using ezVec4I32 = ezVec4Template<ezInt32>;
using ezVec4U32 = ezVec4Template<ezUInt32>;

template <typename Type>
class ezMat3Template;

using ezMat3 = ezMat3Template<float>;
using ezMat3d = ezMat3Template<double>;

template <typename Type>
class ezMat4Template;

using ezMat4 = ezMat4Template<float>;
using ezMat4d = ezMat4Template<double>;

template <typename Type>
struct ezPlaneTemplate;

using ezPlane = ezPlaneTemplate<float>;
using ezPlaned = ezPlaneTemplate<double>;

template <typename Type>
class ezQuatTemplate;

using ezQuat = ezQuatTemplate<float>;
using ezQuatd = ezQuatTemplate<double>;

template <typename Type>
class ezBoundingBoxTemplate;

using ezBoundingBox = ezBoundingBoxTemplate<float>;
using ezBoundingBoxd = ezBoundingBoxTemplate<double>;
using ezBoundingBoxu32 = ezBoundingBoxTemplate<ezUInt32>;

template <typename Type>
class ezBoundingBoxSphereTemplate;

using ezBoundingBoxSphere = ezBoundingBoxSphereTemplate<float>;
using ezBoundingBoxSphered = ezBoundingBoxSphereTemplate<double>;

template <typename Type>
class ezBoundingSphereTemplate;

using ezBoundingSphere = ezBoundingSphereTemplate<float>;
using ezBoundingSphered = ezBoundingSphereTemplate<double>;

template <ezUInt8 DecimalBits>
class ezFixedPoint;

class ezAngle;

template <typename Type>
class ezTransformTemplate;

using ezTransform = ezTransformTemplate<float>;
using ezTransformd = ezTransformTemplate<double>;

class ezColor;
class ezColorLinearUB;
class ezColorGammaUB;

class ezRandom;


/// \brief An enum that allows to select on of the six main axis (positive / negative)
struct EZ_FOUNDATION_DLL ezBasisAxis
{
  using StorageType = ezInt8;

  /// \brief An enum that allows to select on of the six main axis (positive / negative)
  enum Enum : ezInt8
  {
    PositiveX,
    PositiveY,
    PositiveZ,
    NegativeX,
    NegativeY,
    NegativeZ,

    Default = PositiveX
  };

  /// \brief Returns the vector for the given axis. E.g. (1, 0, 0) or (0, -1, 0), etc.
  static ezVec3 GetBasisVector(ezBasisAxis::Enum basisAxis);
  /// \brief Computes a matrix representing the transformation. 'Forward' represents the X axis, 'Right' the Y axis and 'Up' the Z axis.
  static ezMat3 CalculateTransformationMatrix(ezBasisAxis::Enum forwardDir, ezBasisAxis::Enum rightDir, ezBasisAxis::Enum upDir, float fUniformScale = 1.0f, float fScaleX = 1.0f, float fScaleY = 1.0f, float fScaleZ = 1.0f);
  /// \brief Returns a quaternion that rotates from 'identity' to 'axis'
  static ezQuat GetBasisRotation(ezBasisAxis::Enum identity, ezBasisAxis::Enum axis);
  /// \brief Returns a quaternion that rotates from 'PositiveX' to 'axis'
  static ezQuat GetBasisRotation_PosX(ezBasisAxis::Enum axis);
};

