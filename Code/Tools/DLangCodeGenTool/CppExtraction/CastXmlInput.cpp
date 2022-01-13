#  include <Foundation/Math/Math.h>
#  include <Foundation/Math/Quat.h>
#  include <Foundation/Math/Vec2.h>
#  include <Foundation/Math/Vec3.h>
#  include <Foundation/Math/Vec4.h>
#  include <Foundation/Math/Transform.h>
#  include <Foundation/Math/Mat3.h>
#  include <Foundation/Math/Mat4.h>

#ifndef BUILDSYSTEM_BUILDING_DLANGCODEGENTOOL_LIB

#  include <GameEngine/Animation/RotorComponent.h>
#  include <DLangPlugin/Interop/DLangLog_inl.h>

template struct __declspec(dllexport) ezVec2Template<float>;
template struct __declspec(dllexport) ezVec2Template<double>;

template struct __declspec(dllexport) ezVec3Template<float>;
template struct __declspec(dllexport) ezVec3Template<double>;

template struct __declspec(dllexport) ezVec4Template<float>;
template struct __declspec(dllexport) ezVec4Template<double>;

template struct __declspec(dllexport) ezQuatTemplate<float>;
template struct __declspec(dllexport) ezQuatTemplate<double>;

template struct __declspec(dllexport) ezMat3Template<float>;
template struct __declspec(dllexport) ezMat3Template<double>;

template struct __declspec(dllexport) ezMat4Template<float>;
template struct __declspec(dllexport) ezMat4Template<double>;

template struct __declspec(dllexport) ezTransformTemplate<float>;
template struct __declspec(dllexport) ezTransformTemplate<double>;

#endif
