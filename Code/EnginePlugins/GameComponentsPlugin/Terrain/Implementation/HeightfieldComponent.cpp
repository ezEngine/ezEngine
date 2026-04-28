#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Terrain/HeightfieldComponent.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezHeightfieldComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("HeightfieldImage", GetHeightfield, SetHeightfield)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_2D")),
    EZ_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec2(50))),
    EZ_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new ezDefaultValueAttribute(50)),
    EZ_ACCESSOR_PROPERTY("Tesselation", GetTesselation, SetTesselation)->AddAttributes(new ezDefaultValueAttribute(ezVec2U32(128))),
    EZ_ACCESSOR_PROPERTY("TexCoordOffset", GetTexCoordOffset, SetTexCoordOffset)->AddAttributes(new ezDefaultValueAttribute(ezVec2(0))),
    EZ_ACCESSOR_PROPERTY("TexCoordScale", GetTexCoordScale, SetTexCoordScale)->AddAttributes(new ezDefaultValueAttribute(ezVec2(1))),
    EZ_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("ColMeshTesselation", GetColMeshTesselation, SetColMeshTesselation)->AddAttributes(new ezDefaultValueAttribute(ezVec2U32(0))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Terrain"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezHeightfieldComponent::ezHeightfieldComponent() = default;
ezHeightfieldComponent::~ezHeightfieldComponent() = default;

void ezHeightfieldComponent::SetHalfExtents(ezVec2 value)
{
  m_vHalfExtents = value;
  InvalidateMesh();
}

void ezHeightfieldComponent::SetHeight(float value)
{
  m_fHeight = value;
  InvalidateMesh();
}

void ezHeightfieldComponent::SetTexCoordOffset(ezVec2 value)
{
  m_vTexCoordOffset = value;
  InvalidateMesh();
}

void ezHeightfieldComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_hHeightfield;
  s << m_hMaterial;
  s << m_vHalfExtents;
  s << m_fHeight;
  s << m_vTexCoordOffset;
  s << m_vTexCoordScale;
  s << m_vTesselation;
  s << m_vColMeshTesselation;

  // Version 2
  s << m_bGenerateCollision;

  bool m_bIncludeInNavmesh = true; // dummy
  s << m_bIncludeInNavmesh;
}

void ezHeightfieldComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hHeightfield;
  s >> m_hMaterial;
  s >> m_vHalfExtents;
  s >> m_fHeight;
  s >> m_vTexCoordOffset;
  s >> m_vTexCoordScale;
  s >> m_vTesselation;
  s >> m_vColMeshTesselation;

  if (uiVersion >= 2)
  {
    s >> m_bGenerateCollision;

    bool m_bIncludeInNavmesh = true; // dummy
    s >> m_bIncludeInNavmesh;
  }
}

void ezHeightfieldComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<ezMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

void ezHeightfieldComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_bGenerateCollision)
  {
    PushHeightfieldCollider();
  }
}

void ezHeightfieldComponent::PushHeightfieldCollider()
{
  if (!m_hHeightfield.IsValid())
    return;

  ezPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
    return;

  // (0,0) means "use the render mesh resolution". Otherwise use the dedicated collision resolution.
  // m_vTesselation / m_vColMeshTesselation are quad counts; the sample count = quads + 1.
  // Take the smaller axis to keep the grid square, then round down to even.
  const ezVec2U32 colTess = (m_vColMeshTesselation.x == 0 || m_vColMeshTesselation.y == 0) ? m_vTesselation : m_vColMeshTesselation;
  ezUInt32 N = ezMath::Min(colTess.x, colTess.y) + 1;
  if ((N % 2) != 0)
    --N;
  N = ezMath::Max(N, 4u);

  // Scale the extents by the game object's global scale so the collision shape matches the rendered mesh in world space.
  // Jolt's HeightFieldShape supports non-uniform scale via the per-axis mScale vector.
  const ezVec3 vScale = GetOwner()->GetGlobalTransform().m_vScale;
  const ezVec2 vScaledHalfExtents = m_vHalfExtents.CompMul(ezVec2(vScale.x, vScale.y));
  const float fScaledHeight = m_fHeight * vScale.z;

  ezStringBuilder sIdentifier;
  {
    ezUInt64 uiHash = m_hHeightfield.GetResourceIDHash() + m_uiHeightfieldChangeCounter;
    uiHash = ezHashingUtils::xxHash64(&m_vHalfExtents, sizeof(m_vHalfExtents), uiHash);
    uiHash = ezHashingUtils::xxHash64(&m_fHeight, sizeof(m_fHeight), uiHash);
    uiHash = ezHashingUtils::xxHash64(&colTess, sizeof(colTess), uiHash);
    uiHash = ezHashingUtils::xxHash64(&vScale, sizeof(vScale), uiHash);
    sIdentifier.SetFormat("Heightfield:{}", uiHash);
  }

  if (pPhysics->TrySetHeightfieldCollider(GetOwner(), sIdentifier, 0).Succeeded())
    return;

  // Cache miss: build the full height data and create the collider.
  ezResourceLock<ezImageDataResource> pImageData(m_hHeightfield, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Warning("ezHeightfieldComponent: could not load heightmap image '{}' for physics collision.", m_hHeightfield.GetResourceID());
    return;
  }

  const ezImage& heightmap = pImageData->GetDescriptor().m_Image;
  const ezUInt32 uiMaxSamples = ezMath::Min(heightmap.GetWidth(), heightmap.GetHeight());
  N = ezMath::Min(N, uiMaxSamples);
  if ((N % 2) != 0)
    --N;
  if (N < 4)
    N = 4;

  ezPhysicsWorldModuleInterface::HeightfieldColliderData data;
  data.m_uiResolution = N;
  data.m_vHalfExtents = vScaledHalfExtents;
  data.m_Heights.SetCountUninitialized(N * N);
  data.m_MaterialIndices.SetCount((N - 1) * (N - 1), 0);

  const ezColor* pImgData = heightmap.GetPixelPointer<ezColor>();
  const ezUInt32 imgWidth = heightmap.GetWidth();
  const ezUInt32 imgHeight = heightmap.GetHeight();
  const ezVec2 vToNDC = ezVec2(1.0f / (N - 1), 1.0f / (N - 1));

  for (ezUInt32 row = 0; row < N; ++row)
  {
    for (ezUInt32 col = 0; col < N; ++col)
    {
      const ezVec2 ndc = ezVec2((float)col, (float)row).CompMul(vToNDC);
      data.m_Heights[row * N + col] = (ezImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, ezImageAddressMode::Clamp, ndc).r * fScaledHeight) - fScaledHeight;
    }
  }

  // Resolve the surface handle from the material.
  ezMaterialResourceHandle hMaterial = m_hMaterial;
  if (!hMaterial.IsValid())
  {
    hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.ezMaterialAsset
  }
  if (hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMaterial.GetAcquireResult() == ezResourceAcquireResult::Final && !pMaterial->GetSurface().IsEmpty())
    {
      data.m_Surfaces.PushBack(ezResourceManager::LoadResource<ezSurfaceResource>(pMaterial->GetSurface().GetString()));
    }
  }

  pPhysics->CreateHeightfieldCollider(GetOwner(), sIdentifier, data);
}

void ezHeightfieldComponent::OnDeactivated()
{
  ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

ezResult ezHeightfieldComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezHeightfieldComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering());

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh, uiMaterialIndex, uiPartIndex);

    bool bDontCacheYet = false;
    ezRenderData::Category category = ezMaterialResource::GetRenderDataCategory(hMaterial, &bDontCacheYet);

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
  }
}

void ezHeightfieldComponent::SetTexCoordScale(ezVec2 value) // [ property ]
{
  m_vTexCoordScale = value;
  InvalidateMesh();
}

void ezHeightfieldComponent::SetHeightfield(const ezImageDataResourceHandle& hResource)
{
  m_hHeightfield = hResource;
  InvalidateMesh();
}

void ezHeightfieldComponent::SetTesselation(ezVec2U32 value)
{
  m_vTesselation = value;
  InvalidateMesh();
}

void ezHeightfieldComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void ezHeightfieldComponent::SetColMeshTesselation(ezVec2U32 value)
{
  m_vColMeshTesselation = value;
  // don't invalidate the render mesh
}

void ezHeightfieldComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<ezCpuMeshResource>());
}

void ezHeightfieldComponent::InvalidateMesh()
{
  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<ezMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void ezHeightfieldComponent::BuildGeometry(ezGeometry& geom) const
{
  if (!m_hHeightfield.IsValid())
    return;

  EZ_PROFILE_SCOPE("Heightfield: BuildGeometry");

  ezResourceLock<ezImageDataResource> pImageData(m_hHeightfield, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return;
  }

  const ezImage& heightmap = pImageData->GetDescriptor().m_Image;

  const ezUInt32 uiNumVerticesX = ezMath::Clamp(m_vColMeshTesselation.x + 1u, 5u, 512u);
  const ezUInt32 uiNumVerticesY = ezMath::Clamp(m_vColMeshTesselation.y + 1u, 5u, 512u);

  const ezVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
  const ezVec2 vToNDC = ezVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
  const ezVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, 0);

  const ezColor* pImgData = heightmap.GetPixelPointer<ezColor>();
  const ezUInt32 imgWidth = heightmap.GetWidth();
  const ezUInt32 imgHeight = heightmap.GetHeight();

  for (ezUInt32 y = 0; y < uiNumVerticesY; ++y)
  {
    for (ezUInt32 x = 0; x < uiNumVerticesX; ++x)
    {
      const ezVec2 ndc = ezVec2((float)x, (float)y).CompMul(vToNDC);
      const ezVec2 tc = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
      const ezVec2 heightTC = ndc;

      const float fHeightScale = 1.0f - ezImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, ezImageAddressMode::Clamp, heightTC).r;

      const ezVec3 vNewPos = vPosOffset + ezVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize);

      geom.AddVertex(vNewPos, ezVec3(0, 0, 1), tc, ezColor::White);
    }
  }

  ezUInt32 uiVertexIdx = 0;

  for (ezUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
  {
    for (ezUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
    {
      ezUInt32 indices[4];
      indices[0] = uiVertexIdx;
      indices[1] = uiVertexIdx + 1;
      indices[2] = uiVertexIdx + uiNumVerticesX + 1;
      indices[3] = uiVertexIdx + uiNumVerticesX;

      geom.AddPolygon(indices, false);

      ++uiVertexIdx;
    }

    ++uiVertexIdx;
  }
}

ezResult ezHeightfieldComponent::BuildMeshDescriptor(ezMeshResourceDescriptor& desc) const
{
  EZ_PROFILE_SCOPE("Heightfield: GenerateRenderMesh");

  ezResourceLock<ezImageDataResource> pImageData(m_hHeightfield, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return EZ_FAILURE;
  }

  const ezImage& heightmap = pImageData->GetDescriptor().m_Image;

  // Data/Base/Materials/Common/Pattern.ezMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();

  {
    auto& mb = desc.MeshBufferDesc();

    const ezUInt32 uiNumVerticesX = ezMath::Clamp(m_vTesselation.x + 1u, 5u, 1024u);
    const ezUInt32 uiNumVerticesY = ezMath::Clamp(m_vTesselation.y + 1u, 5u, 1024u);
    const ezUInt32 uiNumTriangles = (uiNumVerticesX - 1) * (uiNumVerticesY - 1) * 2;

    mb.AllocateStreams(uiNumVerticesX * uiNumVerticesY, ezGALPrimitiveTopology::Triangles, uiNumTriangles);

    const ezVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
    const ezVec2 vToNDC = ezVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
    const ezVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, -m_fHeight);

    const auto texCoordFormat = mb.GetVertexStreamConfig().GetTexCoordFormat();
    const auto normalFormat = mb.GetVertexStreamConfig().GetNormalFormat();
    const auto tangentFormat = mb.GetVertexStreamConfig().GetTangentFormat();

    // access the vertex data directly
    // this is way more complicated than going through SetVertexData, but it is ~20% faster

    auto positionData = mb.GetPositionData();

    ezUInt32 uiNormalDataStride = 0;
    auto normalData = mb.GetNormalData(&uiNormalDataStride);

    ezUInt32 uiTangentDataStride = 0;
    auto tangentData = mb.GetTangentData(&uiTangentDataStride);

    ezUInt32 uiTexCoordDataStride = 0;
    auto texcoordData = mb.GetTexCoord0Data(&uiTexCoordDataStride);

    ezUInt32 uiVertexIdx = 0;

    const ezColor* pImgData = heightmap.GetPixelPointer<ezColor>();
    const ezUInt32 imgWidth = heightmap.GetWidth();
    const ezUInt32 imgHeight = heightmap.GetHeight();

    for (ezUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (ezUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const ezVec2 ndc = ezVec2((float)x, (float)y).CompMul(vToNDC);
        const ezVec2 tc = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
        const ezVec2 heightTC = ndc;

        const float fHeightScale = ezImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, ezImageAddressMode::Clamp, heightTC).r;

        // complicated but faster
        positionData.GetPtr()[uiVertexIdx] = vPosOffset + ezVec3(ndc.x, ndc.y, fHeightScale).CompMul(vSize);

        const size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiTexCoordDataStride;
        ezMeshBufferUtils::EncodeTexCoord(tc, ezByteArrayPtr(texcoordData.GetPtr() + uiByteOffset, 32), texCoordFormat).IgnoreResult();

        // easier to understand, but slower
        // mb.SetVertexData(0, uiVertexIdx, vPosOffset + ezVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize));
        // ezMeshBufferUtils::EncodeTexCoord(tc, mb.GetVertexData(1, uiVertexIdx), texCoordFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    uiVertexIdx = 0;

    for (ezUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (ezUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const ezInt32 centerIDx = uiVertexIdx;
        ezInt32 leftIDx = uiVertexIdx - 1;
        ezInt32 rightIDx = uiVertexIdx + 1;
        ezInt32 bottomIDx = uiVertexIdx - uiNumVerticesX;
        ezInt32 topIDx = uiVertexIdx + uiNumVerticesX;

        // clamp the indices
        if (x == 0)
          leftIDx = centerIDx;
        if (x + 1 == uiNumVerticesX)
          rightIDx = centerIDx;
        if (y == 0)
          bottomIDx = centerIDx;
        if (y + 1 == uiNumVerticesY)
          topIDx = centerIDx;

        const ezVec3 vPosCenter = *(positionData.GetPtr() + (size_t)centerIDx);
        const ezVec3 vPosLeft = *(positionData.GetPtr() + (size_t)leftIDx);
        const ezVec3 vPosRight = *(positionData.GetPtr() + (size_t)rightIDx);
        const ezVec3 vPosBottom = *(positionData.GetPtr() + (size_t)bottomIDx);
        const ezVec3 vPosTop = *(positionData.GetPtr() + (size_t)topIDx);

        ezVec3 edgeL = vPosLeft - vPosCenter;
        ezVec3 edgeR = vPosRight - vPosCenter;
        ezVec3 edgeB = vPosBottom - vPosCenter;
        ezVec3 edgeT = vPosTop - vPosCenter;

        // rotate edges by 90 degrees, so that they become normals
        ezMath::Swap(edgeL.x, edgeL.z);
        ezMath::Swap(edgeR.x, edgeR.z);
        ezMath::Swap(edgeB.y, edgeB.z);
        ezMath::Swap(edgeT.y, edgeT.z);

        edgeL.z = -edgeL.z;
        edgeR.x = -edgeR.x;
        edgeB.z = -edgeB.z;
        edgeT.y = -edgeT.y;

        // don't normalize the edges first, if they are longer, they shall have more influence
        ezVec3 vNormal(0);
        vNormal += edgeL;
        vNormal += edgeR;
        vNormal += edgeB;
        vNormal += edgeT;
        vNormal.Normalize();

        ezVec3 vTangent = ezVec3(1, 0, 0).CrossRH(vNormal).GetNormalized();

        // complicated but faster
        size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiNormalDataStride;
        ezMeshBufferUtils::EncodeNormal(vNormal, ezByteArrayPtr(normalData.GetPtr() + uiByteOffset, 32), normalFormat).IgnoreResult();

        uiByteOffset = (size_t)uiVertexIdx * (size_t)uiTangentDataStride;
        ezMeshBufferUtils::EncodeTangent(vTangent, 1.0f, ezByteArrayPtr(tangentData.GetPtr() + uiByteOffset, 32), tangentFormat).IgnoreResult();

        // easier to understand, but slower
        // ezMeshBufferUtils::EncodeNormal(ezVec3(0, 0, 1), mb.GetVertexData(2, uiVertexIdx), normalFormat).IgnoreResult();
        // ezMeshBufferUtils::EncodeTangent(ezVec3(1, 0, 0), 1.0f, mb.GetVertexData(3, uiVertexIdx), tangentFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    desc.SetBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(vPosOffset, vPosOffset + vSize.Abs())));

    ezUInt32 uiTriangleIdx = 0;
    uiVertexIdx = 0;

    for (ezUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
    {
      for (ezUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
      {
        mb.SetTriangleIndices(uiTriangleIdx + 0, uiVertexIdx, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX);
        mb.SetTriangleIndices(uiTriangleIdx + 1, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX + 1, uiVertexIdx + uiNumVerticesX);
        uiTriangleIdx += 2;

        ++uiVertexIdx;
      }

      ++uiVertexIdx;
    }
  }

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
  return EZ_SUCCESS;
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezHeightfieldComponent::GenerateMesh() const
{
  if (!m_hHeightfield.IsValid())
    return ezTypedResourceHandle<ResourceType>();

  ezStringBuilder sResourceName;

  {
    ezUInt64 uiSettingsHash = m_hHeightfield.GetResourceIDHash() + m_uiHeightfieldChangeCounter;
    uiSettingsHash = ezHashingUtils::xxHash64(&m_vHalfExtents, sizeof(m_vHalfExtents), uiSettingsHash);
    uiSettingsHash = ezHashingUtils::xxHash64(&m_fHeight, sizeof(m_fHeight), uiSettingsHash);
    uiSettingsHash = ezHashingUtils::xxHash64(&m_vTexCoordOffset, sizeof(m_vTexCoordOffset), uiSettingsHash);
    uiSettingsHash = ezHashingUtils::xxHash64(&m_vTexCoordScale, sizeof(m_vTexCoordScale), uiSettingsHash);
    uiSettingsHash = ezHashingUtils::xxHash64(&m_vTesselation, sizeof(m_vTesselation), uiSettingsHash);

    sResourceName.SetFormat("Heightfield:{}", uiSettingsHash);

    ezTypedResourceHandle<ResourceType> hResource = ezResourceManager::GetExistingResource<ResourceType>(sResourceName);
    if (hResource.IsValid())
      return hResource;
  }

  ezMeshResourceDescriptor desc;
  if (BuildMeshDescriptor(desc).Succeeded())
  {
    return ezResourceManager::CreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
  }

  return ezTypedResourceHandle<ResourceType>();
}

//////////////////////////////////////////////////////////////////////////

ezHeightfieldComponentManager::ezHeightfieldComponentManager(ezWorld* pWorld)
  : ezComponentManager<ComponentType, ezBlockStorageType::Compact>(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezHeightfieldComponentManager::ResourceEventHandler, this));
}

ezHeightfieldComponentManager::~ezHeightfieldComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezHeightfieldComponentManager::ResourceEventHandler, this));
}

void ezHeightfieldComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezHeightfieldComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezHeightfieldComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezImageDataResource>())
  {
    ezImageDataResource* pResource = (ezImageDataResource*)(e.m_pResource);
    const ezUInt32 uiChangeCounter = pResource->GetCurrentResourceChangeCounter();
    ezImageDataResourceHandle hResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hHeightfield == hResource)
      {
        it->m_uiHeightfieldChangeCounter = uiChangeCounter;
        AddToUpdateList(it);
      }
    }
  }
}

void ezHeightfieldComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    ezHeightfieldComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InvalidateMesh();
  }

  m_ComponentsToUpdate.Clear();
}

void ezHeightfieldComponentManager::AddToUpdateList(ezHeightfieldComponent* pComponent)
{
  ezComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == ezInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Terrain_Implementation_HeightfieldComponent);
