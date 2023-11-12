#pragma once

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/Variant.h>

#include <Foundation/Math/Declarations.h>

using ezMathTestType = float;

using ezVec2T = ezVec2Template<ezMathTestType>;                           ///< This is only for testing purposes
using ezVec3T = ezVec3Template<ezMathTestType>;                           ///< This is only for testing purposes
using ezVec4T = ezVec4Template<ezMathTestType>;                           ///< This is only for testing purposes
using ezMat3T = ezMat3Template<ezMathTestType>;                           ///< This is only for testing purposes
using ezMat4T = ezMat4Template<ezMathTestType>;                           ///< This is only for testing purposes
using ezQuatT = ezQuatTemplate<ezMathTestType>;                           ///< This is only for testing purposes
using ezPlaneT = ezPlaneTemplate<ezMathTestType>;                         ///< This is only for testing purposes
using ezBoundingBoxT = ezBoundingBoxTemplate<ezMathTestType>;             ///< This is only for testing purposes
using ezBoundingBoxSphereT = ezBoundingBoxSphereTemplate<ezMathTestType>; ///< This is only for testing purposes
using ezBoundingSphereT = ezBoundingSphereTemplate<ezMathTestType>;       ///< This is only for testing purposes
using ezTransformT = ezTransformTemplate<ezMathTestType>;

#define ezFoundationTest_Plugin1 "ezFoundationTest_Plugin1"
#define ezFoundationTest_Plugin2 "ezFoundationTest_Plugin2"
