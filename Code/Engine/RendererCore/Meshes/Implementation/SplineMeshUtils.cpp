#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Meshes/SplineMeshUtils.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSplineMeshDistributionMode, 1)
  EZ_ENUM_CONSTANTS(ezSplineMeshDistributionMode::FitToSegment, ezSplineMeshDistributionMode::ScaleEvenly, ezSplineMeshDistributionMode::ScaleEvenlyPerSegment)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezSplineMeshPart, ezNoBase, 1, ezRTTIDefaultAllocator<ezSplineMeshPart>)
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_MEMBER_PROPERTY("PaddingFront", m_fPaddingFront),
    EZ_MEMBER_PROPERTY("PaddingBack", m_fPaddingBack),
  }
  EZ_END_PROPERTIES;
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSplineMeshPart::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_hMesh;
  inout_stream << m_fPaddingFront;
  inout_stream << m_fPaddingBack;

  return EZ_SUCCESS;
}

ezResult ezSplineMeshPart::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_hMesh;
  inout_stream >> m_fPaddingFront;
  inout_stream >> m_fPaddingBack;

  return EZ_SUCCESS;
}

ezResult ezSplineMeshPart::ComputeLengthAndOffset(ezVec2& out_vLengthAndOffset) const
{
  if (m_hMesh.IsValid() == false)
    return EZ_FAILURE;

  ezResourceLock<ezMeshResource> pMeshResource(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
  if (pMeshResource.GetAcquireResult() != ezResourceAcquireResult::Final)
    return EZ_FAILURE;

  const auto& bounds = pMeshResource->GetBounds();
  const float fMinExtent = ezMath::Min(bounds.m_vBoxHalfExtents.x, bounds.m_fSphereRadius);

  float fMinX = bounds.m_vCenter.x - fMinExtent;
  float fLength = 2 * fMinExtent;

  out_vLengthAndOffset.Set(fLength, -fMinX);
  return EZ_SUCCESS;
}

ezVec2 ezSplineMeshPart::AddPadding(const ezVec2& vLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack) const
{
  float fLength = vLengthAndOffset.x;
  float fOffset = vLengthAndOffset.y;

  if (bAllowOverlapFront || m_fPaddingFront > 0.0f)
  {
    fLength += m_fPaddingFront;
    fOffset += m_fPaddingFront;
  }

  if (bAllowOverlapBack || m_fPaddingBack > 0.0f)
  {
    fLength += m_fPaddingBack;
  }

  return ezVec2(fLength, fOffset);
}


//////////////////////////////////////////////////////////////////////////

constexpr ezTypeVersion s_SplineMeshDescriptorVersion = 1;

ezResult ezSplineMeshDescriptor::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_SplineMeshDescriptorVersion);

  EZ_SUCCEED_OR_RETURN(m_StartPart.Serialize(inout_stream));
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_MiddleParts));
  EZ_SUCCEED_OR_RETURN(m_EndPart.Serialize(inout_stream));

  inout_stream << m_DistributionMode;
  inout_stream << m_iSeed;

  return EZ_SUCCESS;
}

ezResult ezSplineMeshDescriptor::Deserialize(ezStreamReader& inout_stream)
{
  ezTypeVersion version = inout_stream.ReadVersion(s_SplineMeshDescriptorVersion);

  EZ_SUCCEED_OR_RETURN(m_StartPart.Deserialize(inout_stream));
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_MiddleParts));
  EZ_SUCCEED_OR_RETURN(m_EndPart.Deserialize(inout_stream));

  inout_stream >> m_DistributionMode;
  inout_stream >> m_iSeed;

  return EZ_SUCCESS;
}

ezResult ezSplineMeshDescriptor::GenerateDistribution(const ezSplineComponent& splineComponent, ezDynamicArray<ezMeshResourceHandle>& out_Meshes, ezDynamicArray<ezVec2>& out_scaleOffsets) const
{
  if (m_MiddleParts.IsEmpty() || m_MiddleParts[0].IsValid() == false)
    return EZ_FAILURE;

  if (splineComponent.GetSpline().GetNumControlPoints() < 2)
    return EZ_FAILURE;

  // Gather part offset and length information
  ezVec2 paddedStartLengthAndOffset = ezVec2::MakeZero();
  ezHybridArray<ezVec2, 16> middleLengthAndOffset(ezFrameAllocator::GetCurrentAllocator());
  ezVec2 paddedEndLengthAndOffset = ezVec2::MakeZero();
  {
    if (m_StartPart.IsValid())
    {
      EZ_SUCCEED_OR_RETURN(m_StartPart.ComputeLengthAndOffset(paddedStartLengthAndOffset));
      paddedStartLengthAndOffset = m_StartPart.AddPadding(paddedStartLengthAndOffset, false, true);
    }

    for (ezUInt32 i = 0; i < m_MiddleParts.GetCount(); ++i)
    {
      ezVec2 v = ezVec2::MakeZero();
      EZ_SUCCEED_OR_RETURN(m_MiddleParts[i].ComputeLengthAndOffset(v));
      middleLengthAndOffset.PushBack(v);
    }

    if (m_EndPart.IsValid())
    {
      EZ_SUCCEED_OR_RETURN(m_EndPart.ComputeLengthAndOffset(paddedEndLengthAndOffset));
      paddedEndLengthAndOffset = m_EndPart.AddPadding(paddedEndLengthAndOffset, true, false);
    }
  }

  if (m_DistributionMode == ezSplineMeshDistributionMode::FitToSegment)
  {
    const ezUInt32 uiNumSegments = splineComponent.GetSpline().GetNumSegments();
    ezUInt32 uiSegmentIndex = 0;

    if (m_StartPart.IsValid())
    {
      out_Meshes.PushBack(m_StartPart.m_hMesh);
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromSegment(0, paddedStartLengthAndOffset));

      ++uiSegmentIndex;
    }

    const ezUInt32 uiMiddlePartsEndSegment = uiNumSegments - (m_EndPart.IsValid() ? 1 : 0);
    for (; uiSegmentIndex < uiMiddlePartsEndSegment; ++uiSegmentIndex)
    {
      const float fSegmentLength = splineComponent.GetSegmentLength(uiSegmentIndex);
      const ezUInt32 uiPartIndex = FindBestMiddlePart(fSegmentLength, middleLengthAndOffset, uiSegmentIndex);

      auto& part = m_MiddleParts[uiPartIndex];
      out_Meshes.PushBack(part.m_hMesh);

      const bool bAllowOverlapFront = uiSegmentIndex > 0;
      const bool bAllowOverlapBack = uiSegmentIndex < uiNumSegments - 1;
      const ezVec2 paddedLengthAndOffset = part.AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, bAllowOverlapBack);
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromSegment(uiSegmentIndex, paddedLengthAndOffset));
    }

    if (m_EndPart.IsValid())
    {
      out_Meshes.PushBack(m_EndPart.m_hMesh);
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromSegment(uiSegmentIndex, paddedEndLengthAndOffset));
    }
  }
  else if (m_DistributionMode == ezSplineMeshDistributionMode::ScaleEvenly)
  {
    // First determine the total scale and gather middle part indices
    float fScale = 1.0f;
    ezHybridArray<ezUInt32, 16> middlePartsIndices(ezFrameAllocator::GetCurrentAllocator());
    {
      const float fTotalLength = splineComponent.GetTotalLength();
      float fCurrentLength = 0.0f;

      if (m_StartPart.IsValid())
      {
        fCurrentLength += paddedStartLengthAndOffset.x;
      }

      if (m_EndPart.IsValid())
      {
        fCurrentLength += paddedEndLengthAndOffset.x;
      }

      while (true)
      {
        const ezUInt32 uiPartIndex = 0; // TODO: random
        const bool bAllowOverlapFront = m_StartPart.IsValid() || middlePartsIndices.GetCount() > 0;
        const float fPartLength = m_MiddleParts[uiPartIndex].AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, true).x;

        if (fCurrentLength + fPartLength > fTotalLength)
          break;

        middlePartsIndices.PushBack(uiPartIndex);
        fCurrentLength += fPartLength;
      }

      const float fLastPartLength = fTotalLength - fCurrentLength;
      if (fLastPartLength > 0.1f)
      {
        const ezUInt32 uiPartIndex = FindBestMiddlePart(fLastPartLength, middleLengthAndOffset, static_cast<ezUInt32>(middlePartsIndices.GetCount()));
        middlePartsIndices.PushBack(uiPartIndex);

        fCurrentLength += m_MiddleParts[uiPartIndex].AddPadding(middleLengthAndOffset[uiPartIndex], true, m_EndPart.IsValid()).x;
      }

      fScale = fTotalLength / fCurrentLength;
    }

    float fStartDistance = 0.0f;
    if (m_StartPart.IsValid())
    {
      out_Meshes.PushBack(m_StartPart.m_hMesh);

      const float fEndDistance = fStartDistance + paddedStartLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(splineComponent, fStartDistance, fEndDistance, paddedStartLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    const ezUInt32 uiNumMiddleParts = middlePartsIndices.GetCount();
    for (ezUInt32 i = 0; i < uiNumMiddleParts; ++i)
    {
      const ezUInt32 uiPartIndex = middlePartsIndices[i];
      auto& part = m_MiddleParts[uiPartIndex];

      out_Meshes.PushBack(part.m_hMesh);

      const bool bAllowOverlapFront = m_StartPart.IsValid() || i > 0;
      const bool bAllowOverlapBack = m_EndPart.IsValid() || i < uiNumMiddleParts - 1;
      const ezVec2 paddedLengthAndOffset = part.AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, bAllowOverlapBack);

      const float fEndDistance = fStartDistance + paddedLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(splineComponent, fStartDistance, fEndDistance, paddedLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    if (m_EndPart.IsValid())
    {
      out_Meshes.PushBack(m_EndPart.m_hMesh);

      const float fEndDistance = fStartDistance + paddedEndLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(splineComponent, fStartDistance, fEndDistance, paddedEndLengthAndOffset));
    }
  }
  else if (m_DistributionMode == ezSplineMeshDistributionMode::ScaleEvenlyPerSegment)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return EZ_SUCCESS;
}

ezUInt32 ezSplineMeshDescriptor::FindBestMiddlePart(float fSegmentLength, ezArrayPtr<const ezVec2> middleLengthAndOffset, ezUInt32 uiRandomPos) const
{
  // TODO:
  // EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

ezVec2 ezSplineMeshDescriptor::MakeFinalScaleOffsetFromDistance(const ezSplineComponent& splineComponent, float fStartDistance, float fEndDistance, const ezVec2& vLengthAndOffset) const
{
  const float fStartKey = splineComponent.GetKeyAtDistance(fStartDistance);
  const float fEndKey = splineComponent.GetKeyAtDistance(fEndDistance);
  const float fKeyRange = fEndKey - fStartKey;

  const float fInvLength = 1.0f / vLengthAndOffset.x;
  const float fScale = fKeyRange * fInvLength;
  const float fOffset = (vLengthAndOffset.y * fInvLength) * fKeyRange + fStartKey;

  return ezVec2(fScale, fOffset);
}

ezVec2 ezSplineMeshDescriptor::MakeFinalScaleOffsetFromSegment(ezUInt32 uiSegmentIndex, const ezVec2& vLengthAndOffset) const
{
  const float fInvLength = 1.0f / vLengthAndOffset.x;
  const float fScale = fInvLength;
  const float fOffset = (vLengthAndOffset.y * fInvLength) + uiSegmentIndex;

  return ezVec2(fScale, fOffset);
}
