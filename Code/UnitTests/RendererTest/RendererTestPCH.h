#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

#include <Foundation/Math/Declarations.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

using ezMathTestType = float;

using ezVec2T = ezVec2Template<ezMathTestType>;                     ///< This is only for testing purposes
using ezVec3T = ezVec3Template<ezMathTestType>;                     ///< This is only for testing purposes
using ezVec4T = ezVec4Template<ezMathTestType>;                     ///< This is only for testing purposes
using ezMat3T = ezMat3Template<ezMathTestType>;                     ///< This is only for testing purposes
using ezMat4T = ezMat4Template<ezMathTestType>;                     ///< This is only for testing purposes
using ezQuatT = ezQuatTemplate<ezMathTestType>;                     ///< This is only for testing purposes
using ezPlaneT = ezPlaneTemplate<ezMathTestType>;                   ///< This is only for testing purposes
using ezBoundingBoxT = ezBoundingBoxTemplate<ezMathTestType>;       ///< This is only for testing purposes
using ezBoundingSphereT = ezBoundingSphereTemplate<ezMathTestType>; ///< This is only for testing purposes
