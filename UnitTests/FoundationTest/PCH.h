#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Bitflags.h>

#include <Foundation/Math/Declarations.h>

typedef float ezMathTestType;

typedef ezVec2Template<ezMathTestType> ezVec2T; ///< This is only for testing purposes
typedef ezVec3Template<ezMathTestType> ezVec3T; ///< This is only for testing purposes
typedef ezVec4Template<ezMathTestType> ezVec4T; ///< This is only for testing purposes
typedef ezMat3Template<ezMathTestType> ezMat3T; ///< This is only for testing purposes
typedef ezMat4Template<ezMathTestType> ezMat4T; ///< This is only for testing purposes
typedef ezQuatTemplate<ezMathTestType> ezQuatT; ///< This is only for testing purposes
typedef ezPlaneTemplate<ezMathTestType> ezPlaneT; ///< This is only for testing purposes
typedef ezBoundingBoxTemplate<ezMathTestType> ezBoundingBoxT; ///< This is only for testing purposes
typedef ezBoundingSphereTemplate<ezMathTestType> ezBoundingSphereT; ///< This is only for testing purposes
typedef ezTransformTemplate<ezMathTestType> ezTransformT;
