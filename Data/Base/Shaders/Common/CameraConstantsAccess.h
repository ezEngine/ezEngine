#pragma once

#if EZ_ENABLED(PLATFORM_DX11)

// HLSL

// For stereo support, set this at the beginning of the shader to access the correct values in all camera getters.
static uint s_ActiveCameraEyeIndex = 0;

float4x4 GetCameraToScreenMatrix()  { return CameraToScreenMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetScreenToCameraMatrix()  { return ScreenToCameraMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetWorldToCameraMatrix()   { return WorldToCameraMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetCameraToWorldMatrix()   { return CameraToWorldMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetWorldToScreenMatrix()   { return WorldToScreenMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetScreenToWorldMatrix()   { return ScreenToWorldMatrix[s_ActiveCameraEyeIndex]; }

float3 GetCameraPosition()     { return GetCameraToWorldMatrix()._m03_m13_m23; };
float3 GetCameraDirForwards()  { return GetWorldToCameraMatrix()._m02_m12_m22; };
float3 GetCameraDirRight()     { return GetWorldToCameraMatrix()._m00_m10_m20; };
float3 GetCameraDirUp()        { return GetWorldToCameraMatrix()._m01_m11_m21; };


#elif EZ_ENABLED(PLATFORM_OPENGL)

// GLSL

#else

// C++

#endif
