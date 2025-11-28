#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/SplineMeshComponent.h>

/////////////////////////////////////////////////////////////////////////////

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

  m_Desc.Serialize(s).IgnoreResult();
  s << m_fOffsetY;
  s << m_fOffsetZ;

  // ezMeshComponentBase serialization
  s.WriteArray(m_Materials);
  s << m_Color;
  s << m_vCustomData;
  s << m_fSortingDepthOffset;
}

void ezSplineMeshComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  // Do not call SUPER::DeserializeComponent to avoid deserializing m_hMesh
  ezStreamReader& s = inout_stream.GetStream();

  m_Desc.Deserialize(s).IgnoreResult();
  s >> m_fOffsetY;
  s >> m_fOffsetZ;

  // ezMeshComponentBase de-serialization
  s.ReadArray(m_Materials);
  s >> m_Color;
  s >> m_vCustomData;
  s >> m_fSortingDepthOffset;
}

ezResult ezSplineMeshComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  // TODO:
  return SUPER::GetLocalBounds(ref_bounds, ref_bAlwaysVisible, ref_msg);
}

void ezSplineMeshComponent::SetStartPart(const ezSplineMeshPart& part)
{
  m_Desc.m_StartPart = part;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetMiddleParts(ezArrayPtr<const ezSplineMeshPart> middleParts)
{
  m_Desc.m_MiddleParts = middleParts;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetEndPart(const ezSplineMeshPart& part)
{
  m_Desc.m_EndPart = part;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetDistributionMode(ezEnum<ezSplineMeshDistributionMode> mode)
{
  m_Desc.m_DistributionMode = mode;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::SetSeed(ezInt32 iSeed)
{
  m_Desc.m_iSeed = iSeed;

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

void ezSplineMeshComponent::MiddleParts_SetValue(ezUInt32 uiIndex, const ezSplineMeshPart& value)
{
  m_Desc.m_MiddleParts.EnsureCount(uiIndex + 1);
  m_Desc.m_MiddleParts[uiIndex] = value;

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::MiddleParts_Insert(ezUInt32 uiIndex, const ezSplineMeshPart& value)
{
  m_Desc.m_MiddleParts.InsertAt(uiIndex, value);

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::MiddleParts_Remove(ezUInt32 uiIndex)
{
  m_Desc.m_MiddleParts.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    UpdateSplineMesh();
  }
}

void ezSplineMeshComponent::OnMsgSplineChanged(ezMsgSplineChanged& ref_msg)
{
  UpdateSplineMesh();
}

void ezSplineMeshComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  ezMeshResourceDescriptor splineMeshDesc;
  if (GenerateSplineMesh(splineMeshDesc).Succeeded())
  {
    ezStringBuilder sGuid;
    sGuid.SetFormat("SplineMeshCpu_{}", s_iSplineMeshResources.Increment());

    auto hMeshCpu = ezResourceManager::CreateResource<ezCpuMeshResource>(sGuid, std::move(splineMeshDesc));

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), hMeshCpu);
  }
}

ezResult ezSplineMeshComponent::GenerateSplineMesh(ezMeshResourceDescriptor& out_splineMeshDesc) const
{
  const ezSplineComponent* pSplineComponent = GetSplineComponent();
  if (pSplineComponent == nullptr)
    return EZ_FAILURE;

  ezHybridArray<ezMeshResourceHandle, 16> meshes(ezFrameAllocator::GetCurrentAllocator());
  ezHybridArray<ezVec2, 16> scaleOffsets(ezFrameAllocator::GetCurrentAllocator());
  EZ_SUCCEED_OR_RETURN(m_Desc.GenerateDistribution(*pSplineComponent, meshes, scaleOffsets));

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

  ezHashTable<ezString, ezUInt32> materialMapping(ezFrameAllocator::GetCurrentAllocator());

  for (auto& pMesh : cpuMeshes)
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
    auto pMesh = cpuMeshes[uiMeshIndex];
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
    uiNumPrimitives += meshBufferDesc.GetPrimitiveCount();

    splineMeshBufferDesc.AddStreamConfig(meshBufferDesc.GetVertexStreamConfig());
  }

  subMeshInfos.Sort();

  splineMeshBufferDesc.AllocateStreams(uiNumVertices, topology, uiNumPrimitives);

  ezUInt32 uiFirstVertex = 0;
  ezUInt32 uiIndexByteOffset = 0;
  ezHybridArray<ezUInt32, 16> firstVertexPerMesh(ezFrameAllocator::GetCurrentAllocator());
  firstVertexPerMesh.SetCount(uiNumMeshes);

  for (ezUInt32 uiMeshIndex = 0; uiMeshIndex < uiNumMeshes; ++uiMeshIndex)
  {
    auto& meshBufferDesc = cpuMeshes[uiMeshIndex]->GetDescriptor().MeshBufferDesc();
    const ezVec2& scaleOffset = scaleOffsets[uiMeshIndex];

    for (ezUInt32 v = 0; v < meshBufferDesc.GetVertexCount(); ++v)
    {
      ezVec3 pos = meshBufferDesc.GetPosition(v);
      const float fKey = pos.x * scaleOffset.x + scaleOffset.y;
      const ezTransform transform = pSplineComponent->GetTransformAtKey(fKey);

      pos = transform.TransformPosition(ezVec3(0, pos.y + m_fOffsetY, pos.z + m_fOffsetZ));
      const ezVec3 normal = transform.TransformDirection(meshBufferDesc.GetNormal(v));

      ezVec4 tangent = meshBufferDesc.GetTangent(v);
      tangent = transform.TransformDirection(tangent.GetAsVec3()).GetAsVec4(tangent.w);

      const ezUInt32 uiTargetVertex = uiFirstVertex + v;
      splineMeshBufferDesc.SetPosition(uiTargetVertex, pos);
      splineMeshBufferDesc.SetNormal(uiTargetVertex, normal);
      splineMeshBufferDesc.SetTangent(uiTargetVertex, tangent);
      splineMeshBufferDesc.SetTexCoord0(uiTargetVertex, meshBufferDesc.GetTexCoord0(v));

      if (meshBufferDesc.GetVertexStreamConfig().HasTexCoord1())
      {
        splineMeshBufferDesc.SetTexCoord1(uiTargetVertex, meshBufferDesc.GetTexCoord1(v));
      }

      if (meshBufferDesc.GetVertexStreamConfig().HasColor0())
      {
        splineMeshBufferDesc.SetColor0(uiTargetVertex, meshBufferDesc.GetColor0(v));
      }

      if (meshBufferDesc.GetVertexStreamConfig().HasColor1())
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

    auto& meshBufferDesc = cpuMeshes[subMeshInfo.uiMeshIndex]->GetDescriptor().MeshBufferDesc();
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

void ezSplineMeshComponent::UpdateSplineMesh()
{
  ezMeshResourceDescriptor splineMeshDesc;
  if (GenerateSplineMesh(splineMeshDesc).Failed())
    return;

  ezStringBuilder sGuid;
  sGuid.SetFormat("SplineMesh_{}", s_iSplineMeshResources.Increment());

  auto hSplineMesh = ezResourceManager::CreateResource<ezMeshResource>(sGuid, std::move(splineMeshDesc));

  SetMesh(hSplineMesh);

  // TODO: gen spline collision message
  {
    //ezMsgTransformChanged msg;
    //GetOwner()->SendMessage(msg);
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
