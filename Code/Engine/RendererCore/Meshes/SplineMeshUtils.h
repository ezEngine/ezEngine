#pragma once

#include <RendererCore/Meshes/MeshResource.h>

struct ezSpline;
class ezSplineComponent;

/// \brief Determines how many meshes are distributed along the spline and how they are scaled.
struct ezSplineMeshDistributionMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    FitToSegment,          ///< Each mesh is stretched to fit exactly into one spline segment.
    ScaleEvenly,           ///< Mesh length is used to determine the number of meshes distributed along the whole spline.
    ScaleEvenlyPerSegment, ///< Mesh length is used to determine the number of meshes distributed along each spline segment. This is useful when parts should align with spline nodes.

    Default = ScaleEvenly
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSplineMeshDistributionMode);

struct EZ_RENDERERCORE_DLL ezSplineMeshPart
{
  ezMeshResourceHandle m_hMesh;
  float m_fPaddingFront = 0.0f;
  float m_fPaddingBack = 0.0f;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  bool IsValid() const { return m_hMesh.IsValid(); }
  ezResult ComputeLengthAndOffset(ezVec2& out_vLengthAndOffset) const;
  ezVec2 AddPadding(const ezVec2& vLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack) const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSplineMeshPart);

struct EZ_RENDERERCORE_DLL ezSplineMeshDescriptor
{
  ezSplineMeshPart m_StartPart;
  ezSmallArray<ezSplineMeshPart, 1> m_MiddleParts;
  ezSplineMeshPart m_EndPart;

  ezEnum<ezSplineMeshDistributionMode> m_DistributionMode;
  ezInt32 m_iSeed = -1;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  ezResult GenerateDistribution(const ezSplineComponent& splineComponent, ezDynamicArray<ezMeshResourceHandle>& out_Meshes, ezDynamicArray<ezVec2>& out_scaleOffsets) const;

private:
  ezUInt32 FindBestMiddlePart(float fSegmentLength, ezArrayPtr<const ezVec2> middleLengthAndOffset, ezUInt32 uiRandomPos) const;

  ezVec2 MakeFinalScaleOffsetFromDistance(float fStartDistance, float fEndDistance, const ezVec2& vLengthAndOffset) const;
};
