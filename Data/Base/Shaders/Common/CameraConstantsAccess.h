#pragma once

#if EZ_ENABLED(PLATFORM_SHADER)

// For stereo support, set this at the beginning of the shader to access the correct values in all camera getters.
static uint s_ActiveCameraEyeIndex = 0;

float4x4 GetCameraToScreenMatrix()
{
  return CameraToScreenMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetScreenToCameraMatrix()
{
  return ScreenToCameraMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetWorldToCameraMatrix()
{
  return WorldToCameraMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetCameraToWorldMatrix()
{
  return CameraToWorldMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetWorldToScreenMatrix()
{
  return WorldToScreenMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetScreenToWorldMatrix()
{
  return ScreenToWorldMatrix[s_ActiveCameraEyeIndex];
}

float3 GetCameraPosition()
{
  return GetCameraToWorldMatrix()._m03_m13_m23;
};
float3 GetCameraDirForwards()
{
  return GetCameraToWorldMatrix()._m02_m12_m22;
};
float3 GetCameraDirRight()
{
  return GetCameraToWorldMatrix()._m00_m10_m20;
};
float3 GetCameraDirUp()
{
  return GetCameraToWorldMatrix()._m01_m11_m21;
};

/// Device depth uses the fixed inverse-depth convention: one is the near plane and zero is the far plane.
/// Shader projection matrices use ZeroToOne clip-space depth on all supported rendering backends.
float GetNearPlaneDeviceDepth()
{
  return 1.0f;
}

float GetFarPlaneDeviceDepth()
{
  return 0.0f;
}

/// Returns whether deviceDepth is strictly closer to the camera than referenceDeviceDepth.
bool IsDeviceDepthCloser(float deviceDepth, float referenceDeviceDepth)
{
  return deviceDepth > referenceDeviceDepth;
}

/// Returns whether deviceDepth is strictly farther from the camera than referenceDeviceDepth.
bool IsDeviceDepthFarther(float deviceDepth, float referenceDeviceDepth)
{
  return deviceDepth < referenceDeviceDepth;
}

float SelectClosestDeviceDepth(float leftDeviceDepth, float rightDeviceDepth)
{
  return max(leftDeviceDepth, rightDeviceDepth);
}

float SelectFarthestDeviceDepth(float leftDeviceDepth, float rightDeviceDepth)
{
  return min(leftDeviceDepth, rightDeviceDepth);
}

/// Reconstructs a camera-space position from NDC XY in [-1, +1] and device depth.
float3 ReconstructCameraPositionFromDeviceDepth(float2 ndcXY, float deviceDepth)
{
  float4 cameraPosition = mul(GetScreenToCameraMatrix(), float4(ndcXY, deviceDepth, 1.0f));
  return cameraPosition.xyz / cameraPosition.w;
}

/// Reconstructs a world-space position from NDC XY in [-1, +1] and device depth.
float3 ReconstructWorldPositionFromDeviceDepth(float2 ndcXY, float deviceDepth)
{
  float4 worldPosition = mul(GetScreenToWorldMatrix(), float4(ndcXY, deviceDepth, 1.0f));
  return worldPosition.xyz / worldPosition.w;
}

/// Converts inverse device depth to camera-space Z, not distance from the camera.
/// Uses the perspective projection's reciprocal-depth relation.
float ConvertDeviceDepthToLinearDepth(float deviceDepth)
{
  return 1.0f / (deviceDepth * GetScreenToCameraMatrix()._43 + GetScreenToCameraMatrix()._44);
}

/// Projects camera-space Z to device depth. linearDepth must be finite and positive for perspective projections.
float ConvertLinearDepthToDeviceDepth(float linearDepth)
{
  float4 screenPosition = mul(GetCameraToScreenMatrix(), float4(0.0f, 0.0f, linearDepth, 1.0f));
  return screenPosition.z / screenPosition.w;
}

/// Reconstructs from NDC XY in [-1, +1] and camera-space Z.
/// Assumes a centered perspective projection.
float3 ReconstructPerspectiveCameraPositionFromLinearDepth(float2 ndcXY, float linearDepth)
{
  return float3(float2(GetScreenToCameraMatrix()._11, GetScreenToCameraMatrix()._22) * ndcXY * linearDepth, linearDepth);
}

/// Assumes a centered perspective projection. For general projections use ReconstructCameraPositionFromDeviceDepth().
float3 ReconstructPerspectiveCameraPositionFromDeviceDepth(float2 ndcXY, float deviceDepth)
{
  return ReconstructPerspectiveCameraPositionFromLinearDepth(ndcXY, ConvertDeviceDepthToLinearDepth(deviceDepth));
}

#else

// C++

#endif
