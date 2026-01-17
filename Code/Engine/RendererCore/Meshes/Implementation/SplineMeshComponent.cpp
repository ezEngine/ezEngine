#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/SplineMeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

/////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSplineMeshDistributionMode, 1)
  EZ_ENUM_CONSTANTS(ezSplineMeshDistributionMode::FitToSegment, ezSplineMeshDistributionMode::ScaleEvenly, ezSplineMeshDistributionMode::ScaleEvenlyPerSegment)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

/////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgGenerateSplineMeshCollision);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgGenerateSplineMeshCollision, 1, ezRTTIDefaultAllocator<ezMsgGenerateSplineMeshCollision>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
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

/////////////////////////////////////////////////////////////////////////////

namespace
{
  static EZ_FORCE_INLINE ezUInt32 RandomUInt(ezUInt32 uiMin, ezUInt32 uiMax, int& inout_iRandomPos, ezUInt32 uiSeed)
  {
    ezSimdVec4u res = ezSimdVec4u::Truncate(ezSimdRandom::FloatMinMax(ezSimdVec4i(inout_iRandomPos), ezSimdVec4f((float)uiMin), ezSimdVec4f((float)uiMax), ezSimdVec4u(uiSeed)));
    ++inout_iRandomPos;
    return res.x();
  }

  static EZ_FORCE_INLINE ezVec2 MakeFinalScaleOffsetFromDistance(float fStartDistance, float fEndDistance, const ezVec2& vLengthAndOffset)
  {
    const float fRange = fEndDistance - fStartDistance;

    const float fInvLength = 1.0f / vLengthAndOffset.x;
    const float fScale = fRange * fInvLength;
    const float fOffset = (vLengthAndOffset.y * fInvLength) * fRange + fStartDistance;

    return ezVec2(fScale, fOffset);
  }
} // namespace

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSplineMeshComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("StartPart", GetStartPart, SetStartPart),
    EZ_ARRAY_ACCESSOR_PROPERTY("MiddleParts", MiddleParts_GetCount, MiddleParts_GetValue, MiddleParts_SetValue, MiddleParts_Insert, MiddleParts_Remove),
    EZ_ACCESSOR_PROPERTY("EndPart", GetEndPart, SetEndPart),
    EZ_ENUM_ACCESSOR_PROPERTY("DistributionMode", ezSplineMeshDistributionMode, GetDistributionMode, SetDistributionMode),
    EZ_ACCESSOR_PROPERTY("Seed", GetSeed, SetSeed)->AddAttributes(new ezDefaultValueAttribute(-1), new ezClampValueAttribute(-1, ezVariant()), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("OffsetY", GetOffsetY, SetOffsetY),
    EZ_ACCESSOR_PROPERTY("OffsetZ", GetOffsetZ, SetOffsetZ),

    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new ezDefaultValueAttribute(ezVec4(0, 1, 0, 1))),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSplineChanged, OnMsgSplineChanged),
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAtomicInteger32 s_iSplineMeshResources;

ezSplineMeshComponent::ezSplineMeshComponent() = default;
ezSplineMeshComponent::~ezSplineMeshComponent() = default;

void ezSplineMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateSplineMesh();
}

void ezSplineMeshComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
}

void ezSplineMeshComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  // Do not call SUPER::SerializeComponent to avoid serializing m_hMesh
  ezStreamWriter& s = inout_stream.GetStream();

  m_StartPart.Serialize(s).IgnoreResult();
  s.WriteArray(m_MiddleParts).IgnoreResult();
  m_EndPart.Serialize(s).IgnoreResult();

  s << m_DistributionMode;
  s << m_iSeed;

  s << m_fOffsetY;
  s << m_fOffsetZ;
  s << m_uiStableId;

  // ezMeshComponentBase serialization
  s.WriteArray(m_Materials).IgnoreResult();
  s << m_Color;
  s << m_vCustomData;
  s << m_fSortingDepthOffset;
}

void ezSplineMeshComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  // Do not call SUPER::DeserializeComponent to avoid deserializing m_hMesh
  ezStreamReader& s = inout_stream.GetStream();

  m_StartPart.Deserialize(s).IgnoreResult();
  s.ReadArray(m_MiddleParts).IgnoreResult();
  m_EndPart.Deserialize(s).IgnoreResult();

  s >> m_DistributionMode;
  s >> m_iSeed;

  s >> m_fOffsetY;
  s >> m_fOffsetZ;
  s >> m_uiStableId;

  // ezMeshComponentBase de-serialization
  s.ReadArray(m_Materials).IgnoreResult();
  s >> m_Color;
  s >> m_vCustomData;
  s >> m_fSortingDepthOffset;
}

void ezSplineMeshComponent::SetStartPart(const ezSplineMeshPart& part)
{
  m_StartPart = part;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetMiddleParts(ezArrayPtr<const ezSplineMeshPart> middleParts)
{
  m_MiddleParts = middleParts;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetEndPart(const ezSplineMeshPart& part)
{
  m_EndPart = part;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetDistributionMode(ezEnum<ezSplineMeshDistributionMode> mode)
{
  m_DistributionMode = mode;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetSeed(ezInt32 iSeed)
{
  m_iSeed = iSeed;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetOffsetY(float fOffsetY)
{
  if (m_fOffsetY == fOffsetY)
    return;

  m_fOffsetY = fOffsetY;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetOffsetZ(float fOffsetZ)
{
  if (m_fOffsetZ == fOffsetZ)
    return;

  m_fOffsetZ = fOffsetZ;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

ezResult ezSplineMeshComponent::GenerateSplineMeshDesc(const ezSpline& spline, const ezArrayMap<float, float>& distanceToKey, ezArrayPtr<ezCpuMeshResource*> meshes, ezArrayPtr<ezVec2> scaleOffsets, float fLocalOffsetY, float fLocalOffsetZ, ezMeshResourceDescriptor& out_splineMeshDesc)
{
  ezHashTable<ezString, ezUInt32> materialMapping;

  for (auto& pMesh : meshes)
  {
    for (auto& mat : pMesh->GetDescriptor().GetMaterials())
    {
      if (materialMapping.Contains(mat.m_sPath) == false)
      {
        materialMapping.Insert(mat.m_sPath, materialMapping.GetCount());
      }
    }

    for (auto it : materialMapping)
    {
      out_splineMeshDesc.SetMaterial(it.Value(), it.Key());
    }
  }

  struct SubMeshInfo
  {
    ezUInt32 uiMaterialIndex = 0;
    ezUInt32 uiMeshIndex = 0;
    ezUInt32 uiFirstPrimitive = 0;
    ezUInt32 uiPrimitiveCount = 0;

    bool operator<(const SubMeshInfo& rhs) const
    {
      if (uiMaterialIndex != rhs.uiMaterialIndex)
        return uiMaterialIndex < rhs.uiMaterialIndex;
      if (uiMeshIndex != rhs.uiMeshIndex)
        return uiMeshIndex < rhs.uiMeshIndex;
      return uiFirstPrimitive < rhs.uiFirstPrimitive;
    }
  };

  auto& splineMeshBufferDesc = out_splineMeshDesc.MeshBufferDesc();
  const ezUInt32 uiNumMeshes = meshes.GetCount();

  constexpr auto topology = ezGALPrimitiveTopology::Triangles;
  ezUInt32 uiNumVertices = 0;
  ezUInt32 uiNumPrimitives = 0;
  ezHybridArray<SubMeshInfo, 32> subMeshInfos(ezFrameAllocator::GetCurrentAllocator());
  for (ezUInt32 uiMeshIndex = 0; uiMeshIndex < uiNumMeshes; ++uiMeshIndex)
  {
    auto pMesh = meshes[uiMeshIndex];
    auto materials = pMesh->GetDescriptor().GetMaterials();

    for (auto& subMesh : pMesh->GetDescriptor().GetSubMeshes())
    {
      auto& info = subMeshInfos.ExpandAndGetRef();
      info.uiMaterialIndex = materialMapping[materials[subMesh.m_uiMaterialIndex].m_sPath];
      info.uiMeshIndex = uiMeshIndex;
      info.uiFirstPrimitive = subMesh.m_uiFirstPrimitive;
      info.uiPrimitiveCount = subMesh.m_uiPrimitiveCount;
    }

    auto& meshBufferDesc = pMesh->GetDescriptor().MeshBufferDesc();
    uiNumVertices += meshBufferDesc.GetVertexCount();
    if (meshBufferDesc.HasIndexBuffer())
    {
      uiNumPrimitives += meshBufferDesc.GetPrimitiveCount();
    }

    splineMeshBufferDesc.AddStreamConfig(meshBufferDesc.GetVertexStreamConfig());
  }

  subMeshInfos.Sort();

  splineMeshBufferDesc.AllocateStreams(uiNumVertices, topology, uiNumPrimitives);

  ezUInt32 uiFirstVertex = 0;
  ezUInt32 uiIndexByteOffset = 0;
  ezHybridArray<ezUInt32, 16> firstVertexPerMesh(ezFrameAllocator::GetCurrentAllocator());
  firstVertexPerMesh.SetCount(uiNumMeshes);

  const bool bHasNTT = splineMeshBufferDesc.GetVertexStreamConfig().HasNormalTangentAndTexCoord0();
  const bool bHasTexCoord1 = splineMeshBufferDesc.GetVertexStreamConfig().HasTexCoord1();
  const bool bHasColor0 = splineMeshBufferDesc.GetVertexStreamConfig().HasColor0();
  const bool bHasColor1 = splineMeshBufferDesc.GetVertexStreamConfig().HasColor1();

  for (ezUInt32 uiMeshIndex = 0; uiMeshIndex < uiNumMeshes; ++uiMeshIndex)
  {
    auto& meshBufferDesc = meshes[uiMeshIndex]->GetDescriptor().MeshBufferDesc();
    const ezVec2& scaleOffset = scaleOffsets[uiMeshIndex];

    for (ezUInt32 v = 0; v < meshBufferDesc.GetVertexCount(); ++v)
    {
      ezVec3 pos = meshBufferDesc.GetPosition(v);
      const float fDistance = pos.x * scaleOffset.x + scaleOffset.y;
      const float fKey = ezSplineComponent::GetKeyAtDistanceHelper(distanceToKey, fDistance);
      const ezTransform transform = ezSimdConversion::ToTransform(spline.EvaluateTransform(fKey));

      const ezUInt32 uiTargetVertex = uiFirstVertex + v;

      pos = transform.TransformPosition(ezVec3(0, pos.y + fLocalOffsetY, pos.z + fLocalOffsetZ));
      splineMeshBufferDesc.SetPosition(uiTargetVertex, pos);

      if (bHasNTT)
      {
        const ezVec3 normal = transform.TransformDirection(meshBufferDesc.GetNormal(v));

        ezVec4 tangent = meshBufferDesc.GetTangent(v);
        tangent = transform.TransformDirection(tangent.GetAsVec3()).GetAsVec4(tangent.w);

        splineMeshBufferDesc.SetNormal(uiTargetVertex, normal);
        splineMeshBufferDesc.SetTangent(uiTargetVertex, tangent);
        splineMeshBufferDesc.SetTexCoord0(uiTargetVertex, meshBufferDesc.GetTexCoord0(v));
      }

      if (bHasTexCoord1)
      {
        splineMeshBufferDesc.SetTexCoord1(uiTargetVertex, meshBufferDesc.GetTexCoord1(v));
      }

      if (bHasColor0)
      {
        splineMeshBufferDesc.SetColor0(uiTargetVertex, meshBufferDesc.GetColor0(v));
      }

      if (bHasColor1)
      {
        splineMeshBufferDesc.SetColor1(uiTargetVertex, meshBufferDesc.GetColor1(v));
      }
    }

    firstVertexPerMesh[uiMeshIndex] = uiFirstVertex;
    uiFirstVertex += meshBufferDesc.GetVertexCount();
  }

  ezUInt32 uiTargetIndex = 0;
  ezUInt32 uiTargetFirstPrimitive = 0;
  ezUInt32 uiTargetPrimitiveCount = 0;
  auto& targetIndices = splineMeshBufferDesc.GetIndexBufferData();

  for (ezUInt32 uiSubMeshInfoIndex = 0; uiSubMeshInfoIndex < subMeshInfos.GetCount(); ++uiSubMeshInfoIndex)
  {
    auto& subMeshInfo = subMeshInfos[uiSubMeshInfoIndex];

    auto& meshBufferDesc = meshes[subMeshInfo.uiMeshIndex]->GetDescriptor().MeshBufferDesc();
    if (meshBufferDesc.HasIndexBuffer())
    {
      auto sourceIndices = meshBufferDesc.GetIndexBufferData();
      EZ_ASSERT_DEV(meshBufferDesc.Uses32BitIndices() == splineMeshBufferDesc.Uses32BitIndices(), "Index buffer format mismatch");

      const ezUInt32 uiFirstVertex = firstVertexPerMesh[subMeshInfo.uiMeshIndex];
      const ezUInt32 uiSourceFirstIndex = ezGALPrimitiveTopology::GetIndexCount(topology, subMeshInfo.uiFirstPrimitive);
      const ezUInt32 uiSourceIndexCount = ezGALPrimitiveTopology::GetIndexCount(topology, subMeshInfo.uiPrimitiveCount);

      uiTargetPrimitiveCount += subMeshInfo.uiPrimitiveCount;

      if (splineMeshBufferDesc.Uses32BitIndices())
      {
        auto pSourceIndices32 = reinterpret_cast<const ezUInt32*>(sourceIndices.GetPtr());
        auto pTargetIndices32 = reinterpret_cast<ezUInt32*>(targetIndices.GetData());
        for (ezUInt32 i = 0; i < uiSourceIndexCount; ++i)
        {
          pTargetIndices32[uiTargetIndex] = pSourceIndices32[i + uiSourceFirstIndex] + uiFirstVertex;
          ++uiTargetIndex;
        }
      }
      else
      {
        auto pSourceIndices16 = reinterpret_cast<const ezUInt16*>(sourceIndices.GetPtr());
        auto pTargetIndices16 = reinterpret_cast<ezUInt16*>(targetIndices.GetData());
        for (ezUInt32 i = 0; i < uiSourceIndexCount; ++i)
        {
          pTargetIndices16[uiTargetIndex] = pSourceIndices16[i + uiSourceFirstIndex] + uiFirstVertex;
          ++uiTargetIndex;
        }
      }
    }

    if (uiSubMeshInfoIndex == subMeshInfos.GetCount() - 1 || subMeshInfos[uiSubMeshInfoIndex + 1].uiMaterialIndex != subMeshInfo.uiMaterialIndex)
    {
      out_splineMeshDesc.AddSubMesh(uiTargetPrimitiveCount, uiTargetFirstPrimitive, subMeshInfo.uiMaterialIndex);

      uiTargetFirstPrimitive += uiTargetPrimitiveCount;
      uiTargetPrimitiveCount = 0;
    }
  }

  out_splineMeshDesc.ComputeBounds();

  return EZ_SUCCESS;
}

void ezSplineMeshComponent::MiddleParts_SetValue(ezUInt32 uiIndex, const ezSplineMeshPart& value)
{
  m_MiddleParts.EnsureCount(uiIndex + 1);
  m_MiddleParts[uiIndex] = value;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::MiddleParts_Insert(ezUInt32 uiIndex, const ezSplineMeshPart& value)
{
  m_MiddleParts.InsertAt(uiIndex, value);

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::MiddleParts_Remove(ezUInt32 uiIndex)
{
  m_MiddleParts.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiStableId = ezHashingUtils::xxHash64(&node.GetGuid(), sizeof(ezUuid));
}

void ezSplineMeshComponent::OnMsgSplineChanged(ezMsgSplineChanged& ref_msg)
{
  // An invalid change counter indicates that this msg came from a spline node before the actual spline has been updated. Ignore that here.
  if (ref_msg.m_uiChangeCounter == ezInvalidIndex || ref_msg.m_uiChangeCounter == m_uiLastSplineChangeCounter)
    return;

  UpdateSplineMesh();
}

void ezSplineMeshComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  auto hMesh = GetMesh();
  if (hMesh.IsValid())
  {
    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), ezResourceManager::LoadResource<ezCpuMeshResource>(hMesh.GetResourceID()));
    return;
  }

  const ezSplineComponent* pSplineComponent = GetSplineComponent();
  if (pSplineComponent == nullptr)
    return;

  ezMeshResourceDescriptor splineMeshDesc;
  if (GenerateSplineMesh(*pSplineComponent, splineMeshDesc).Succeeded())
  {
    ezStringBuilder sGuid;
    sGuid.SetFormat("SplineMeshCpu_{}", s_iSplineMeshResources.Increment());

    auto hMeshCpu = ezResourceManager::CreateResource<ezCpuMeshResource>(sGuid, std::move(splineMeshDesc));

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), hMeshCpu);
  }
}

void ezSplineMeshComponent::GenerateMeshPath(const ezSplineComponent& splineComponent, ezStringBuilder& out_sSplineMeshPath) const
{
  const ezUInt64 uiStableSplineId = ezHashingUtils::xxHash64(&splineComponent.GetUuid(), sizeof(ezUuid));

  out_sSplineMeshPath.SetFormat(":project/AssetCache/Generated/SplineMesh_{}_{}.ezBinMesh", ezArgU(m_uiStableId, 16, true, 16, true), ezArgU(uiStableSplineId, 16, true, 16, true));
}

ezResult ezSplineMeshComponent::GenerateDistribution(const ezSplineComponent& splineComponent, ezDynamicArray<ezMeshResourceHandle>& out_Meshes, ezDynamicArray<ezVec2>& out_scaleOffsets) const
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

  int iRandomPos = 46237;
  ezUInt32 uiSeed = m_iSeed < 0 ? GetOwner()->GetStableRandomSeed() : static_cast<ezUInt32>(m_iSeed);

  auto GenerateEvenDistribution = [&](float fStartDistance, float fTotalLength, bool bAllowStartPart, bool bAllowEndPart)
  {
    const bool bHasStartPart = bAllowStartPart && m_StartPart.IsValid();
    const bool bHasEndPart = bAllowEndPart && m_EndPart.IsValid();

    // First determine the total scale and gather middle part indices
    float fScale = 1.0f;
    ezHybridArray<ezUInt32, 16> middlePartsIndices(ezFrameAllocator::GetCurrentAllocator());
    {
      float fCurrentLength = 0.0f;

      if (bHasStartPart)
      {
        fCurrentLength += paddedStartLengthAndOffset.x;
      }

      if (bHasEndPart)
      {
        fCurrentLength += paddedEndLengthAndOffset.x;
      }

      while (true)
      {
        const ezUInt32 uiPartIndex = RandomUInt(0, m_MiddleParts.GetCount(), iRandomPos, uiSeed);
        const bool bAllowOverlapFront = bHasStartPart || middlePartsIndices.GetCount() > 0;
        const float fPartLength = m_MiddleParts[uiPartIndex].AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, true).x;

        if (fCurrentLength + fPartLength > fTotalLength)
          break;

        middlePartsIndices.PushBack(uiPartIndex);
        fCurrentLength += fPartLength;
      }

      const float fLastPartLength = fTotalLength - fCurrentLength;
      if (fLastPartLength > 0.1f)
      {
        const bool bAllowOverlapFront = bHasStartPart || middlePartsIndices.GetCount() > 0;
        const bool bAllowOverlapBack = bHasEndPart;
        const ezUInt32 uiPartIndex = FindBestMiddlePart(fLastPartLength, middleLengthAndOffset, bAllowOverlapFront, bAllowOverlapBack, iRandomPos, uiSeed);
        middlePartsIndices.PushBack(uiPartIndex);

        fCurrentLength += m_MiddleParts[uiPartIndex].AddPadding(middleLengthAndOffset[uiPartIndex], true, m_EndPart.IsValid()).x;
      }

      fScale = fTotalLength / fCurrentLength;
    }

    if (bHasStartPart)
    {
      out_Meshes.PushBack(m_StartPart.m_hMesh);

      const float fEndDistance = fStartDistance + paddedStartLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedStartLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    const ezUInt32 uiNumMiddleParts = middlePartsIndices.GetCount();
    for (ezUInt32 i = 0; i < uiNumMiddleParts; ++i)
    {
      const ezUInt32 uiPartIndex = middlePartsIndices[i];
      auto& part = m_MiddleParts[uiPartIndex];

      out_Meshes.PushBack(part.m_hMesh);

      const bool bAllowOverlapFront = m_StartPart.IsValid() || i > 0;
      const bool bAllowOverlapBack = bHasEndPart || i < uiNumMiddleParts - 1;
      const ezVec2 paddedLengthAndOffset = part.AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, bAllowOverlapBack);

      const float fEndDistance = fStartDistance + paddedLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    if (bHasEndPart)
    {
      out_Meshes.PushBack(m_EndPart.m_hMesh);

      const float fEndDistance = fStartDistance + paddedEndLengthAndOffset.x * fScale;
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedEndLengthAndOffset));
    }
  };

  if (m_DistributionMode == ezSplineMeshDistributionMode::ScaleEvenly)
  {
    const float fStartDistance = 0.0f;
    const float fTotalLength = splineComponent.GetTotalLength();
    const bool bAllowStartPart = !splineComponent.GetClosed();
    const bool bAllowEndPart = !splineComponent.GetClosed();
    GenerateEvenDistribution(fStartDistance, fTotalLength, bAllowStartPart, bAllowEndPart);
  }
  else if (m_DistributionMode == ezSplineMeshDistributionMode::ScaleEvenlyPerSegment)
  {
    const bool bClosed = splineComponent.GetClosed();
    const ezUInt32 uiNumSegments = splineComponent.GetSpline().GetNumSegments();
    float fStartDistance = 0.0f;
    for (ezUInt32 uiSegmentIndex = 0; uiSegmentIndex < uiNumSegments; ++uiSegmentIndex)
    {
      const float fSegmentLength = splineComponent.GetSegmentLength(uiSegmentIndex);
      const bool bAllowStartPart = !bClosed && (uiSegmentIndex == 0);
      const bool bAllowEndPart = !bClosed && (uiSegmentIndex == uiNumSegments - 1);
      GenerateEvenDistribution(fStartDistance, fSegmentLength, bAllowStartPart, bAllowEndPart);

      fStartDistance += fSegmentLength;
    }
  }
  else if (m_DistributionMode == ezSplineMeshDistributionMode::FitToSegment)
  {
    const bool bClosed = splineComponent.GetClosed();
    const bool bHasStartPart = !bClosed && m_StartPart.IsValid();
    const bool bHasEndPart = !bClosed && m_EndPart.IsValid();
    const ezUInt32 uiNumSegments = splineComponent.GetSpline().GetNumSegments();
    ezUInt32 uiSegmentIndex = 0;

    float fStartDistance = 0.0f;
    if (bHasStartPart)
    {
      out_Meshes.PushBack(m_StartPart.m_hMesh);

      const float fEndDistance = fStartDistance + splineComponent.GetSegmentLength(uiSegmentIndex);
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedStartLengthAndOffset));

      ++uiSegmentIndex;
      fStartDistance = fEndDistance;
    }

    const ezUInt32 uiMiddlePartsEndSegment = uiNumSegments - (bHasEndPart ? 1 : 0);
    for (; uiSegmentIndex < uiMiddlePartsEndSegment; ++uiSegmentIndex)
    {
      const float fSegmentLength = splineComponent.GetSegmentLength(uiSegmentIndex);
      const bool bAllowOverlapFront = uiSegmentIndex > 0;
      const bool bAllowOverlapBack = uiSegmentIndex < uiNumSegments - 1;
      const ezUInt32 uiPartIndex = FindBestMiddlePart(fSegmentLength, middleLengthAndOffset, bAllowOverlapFront, bAllowOverlapBack, iRandomPos, uiSeed);

      auto& part = m_MiddleParts[uiPartIndex];
      out_Meshes.PushBack(part.m_hMesh);

      const float fEndDistance = fStartDistance + fSegmentLength;
      const ezVec2 paddedLengthAndOffset = part.AddPadding(middleLengthAndOffset[uiPartIndex], bAllowOverlapFront, bAllowOverlapBack);
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedLengthAndOffset));

      fStartDistance = fEndDistance;
    }

    if (bHasEndPart)
    {
      out_Meshes.PushBack(m_EndPart.m_hMesh);

      const float fEndDistance = splineComponent.GetTotalLength();
      out_scaleOffsets.PushBack(MakeFinalScaleOffsetFromDistance(fStartDistance, fEndDistance, paddedEndLengthAndOffset));
    }
  }

  return EZ_SUCCESS;
}

ezUInt32 ezSplineMeshComponent::FindBestMiddlePart(float fRequestedLength, ezArrayPtr<const ezVec2> middleLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack, int& inout_iRandomPos, ezUInt32 uiSeed) const
{
  float fBestDiff = ezMath::MaxValue<float>();
  ezHybridArray<ezUInt32, 8> candidateIndices;

  EZ_ASSERT_DEBUG(m_MiddleParts.GetCount() == middleLengthAndOffset.GetCount(), "Mismatched array sizes");
  for (ezUInt32 i = 0; i < m_MiddleParts.GetCount(); ++i)
  {
    const float fPartLength = m_MiddleParts[i].AddPadding(middleLengthAndOffset[i], bAllowOverlapFront, bAllowOverlapBack).x;
    const float fDiff = ezMath::Abs(fPartLength - fRequestedLength);
    if (ezMath::IsEqual(fDiff, fBestDiff, 0.01f))
    {
      candidateIndices.PushBack(i);
    }
    else if (fDiff < fBestDiff)
    {
      fBestDiff = fDiff;

      candidateIndices.Clear();
      candidateIndices.PushBack(i);
    }
  }

  if (candidateIndices.GetCount() == 1)
    return candidateIndices[0];

  const ezUInt32 uiRandom = RandomUInt(0, candidateIndices.GetCount(), inout_iRandomPos, uiSeed);
  return candidateIndices[uiRandom];
}

ezResult ezSplineMeshComponent::GenerateSplineMesh(const ezSplineComponent& splineComponent, ezMeshResourceDescriptor& out_splineMeshDesc, ezMsgGenerateSplineMeshCollision* out_pMsg /*= nullptr*/) const
{
  ezHybridArray<ezMeshResourceHandle, 16> meshes(ezFrameAllocator::GetCurrentAllocator());
  ezHybridArray<ezVec2, 16> scaleOffsets(ezFrameAllocator::GetCurrentAllocator());
  EZ_SUCCEED_OR_RETURN(GenerateDistribution(splineComponent, meshes, scaleOffsets));

  if (out_pMsg != nullptr)
  {
    out_pMsg->m_hSplineComponent = splineComponent.GetHandle();
    out_pMsg->m_RenderMeshes = meshes;
    out_pMsg->m_ScaleOffsets = scaleOffsets;
    out_pMsg->m_fLocalOffsetY = m_fOffsetY;
    out_pMsg->m_fLocalOffsetZ = m_fOffsetZ;
  }

  ezHybridArray<ezCpuMeshResource*, 16> cpuMeshes(ezFrameAllocator::GetCurrentAllocator());

  for (auto& hMesh : meshes)
  {
    auto hMeshCpu = ezResourceManager::LoadResource<ezCpuMeshResource>(hMesh.GetResourceID());
    ezCpuMeshResource* pMeshCpu = ezResourceManager::BeginAcquireResource<ezCpuMeshResource>(hMeshCpu, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    EZ_ASSERT_DEV(pMeshCpu != nullptr, "Failed to load cpu mesh resource for spline mesh generation");
    cpuMeshes.PushBack(pMeshCpu);
  }

  EZ_SCOPE_EXIT(
    for (auto pMeshCpu : cpuMeshes) {
      ezResourceManager::EndAcquireResource(pMeshCpu);
    });

  return GenerateSplineMeshDesc(splineComponent.GetSpline(), splineComponent.GetDistanceToKeyRemapping(), cpuMeshes, scaleOffsets, m_fOffsetY, m_fOffsetZ, out_splineMeshDesc);
}

void ezSplineMeshComponent::UpdateSplineMesh()
{
  const ezSplineComponent* pSplineComponent = GetSplineComponent();
  if (pSplineComponent == nullptr)
    return;

  ezStringBuilder sMeshPath;
  GenerateMeshPath(*pSplineComponent, sMeshPath);

  if (GetUniqueID() != ezInvalidIndex)
  {
    // Only generate in the editor
    ezMeshResourceDescriptor splineMeshDesc;
    ezMsgGenerateSplineMeshCollision genColMsg;
    if (GenerateSplineMesh(*pSplineComponent, splineMeshDesc, &genColMsg).Failed())
      return;

    m_uiLastSplineChangeCounter = pSplineComponent->GetChangeCounter();

    ezFileWriter file;
    if (file.Open(sMeshPath).Succeeded())
    {
      ezAssetFileHeader header;
      header.SetFileHashAndVersion(m_uiStableId, 0);
      header.Write(file).IgnoreResult();

      splineMeshDesc.Save(file);
    }
    else
    {
      ezLog::Error("Failed to save generated spline mesh to '{}'", sMeshPath);
      return;
    }

    GetOwner()->PostMessage(genColMsg, ezTime::MakeZero());
  }

  auto hSplineMesh = ezResourceManager::LoadResource<ezMeshResource>(sMeshPath);
  if (GetMesh() == hSplineMesh)
  {
    ezResourceManager::ReloadResource(hSplineMesh, true);
  }
  else
  {
    SetMesh(hSplineMesh);
  }
}

const ezSplineComponent* ezSplineMeshComponent::GetSplineComponent() const
{
  const ezGameObject* pObject = GetOwner();
  while (pObject != nullptr)
  {
    const ezSplineComponent* pSplineComponent = nullptr;
    if (pObject->TryGetComponentOfBaseType(pSplineComponent))
    {
      return pSplineComponent;
    }
    pObject = pObject->GetParent();
  }

  return nullptr;
}
