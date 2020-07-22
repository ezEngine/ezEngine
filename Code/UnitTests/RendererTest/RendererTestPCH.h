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
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

typedef float ezMathTestType;

typedef ezVec2Template<ezMathTestType> ezVec2T;                     ///< This is only for testing purposes
typedef ezVec3Template<ezMathTestType> ezVec3T;                     ///< This is only for testing purposes
typedef ezVec4Template<ezMathTestType> ezVec4T;                     ///< This is only for testing purposes
typedef ezMat3Template<ezMathTestType> ezMat3T;                     ///< This is only for testing purposes
typedef ezMat4Template<ezMathTestType> ezMat4T;                     ///< This is only for testing purposes
typedef ezQuatTemplate<ezMathTestType> ezQuatT;                     ///< This is only for testing purposes
typedef ezPlaneTemplate<ezMathTestType> ezPlaneT;                   ///< This is only for testing purposes
typedef ezBoundingBoxTemplate<ezMathTestType> ezBoundingBoxT;       ///< This is only for testing purposes
typedef ezBoundingSphereTemplate<ezMathTestType> ezBoundingSphereT; ///< This is only for testing purposes
