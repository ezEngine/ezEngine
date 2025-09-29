#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Terrain/HeightfieldComponent.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
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
    EZ_ACCESSOR_PROPERTY("ColMeshTesselation", GetColMeshTesselation, SetColMeshTesselation)->AddAttributes(new ezDefaultValueAttribute(ezVec2U32(64))),    
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
    EZ_MESSAGE_HANDLER(ezMsgBuildStaticMesh, OnBuildStaticMesh),
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

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().HasMirrorScaling() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = ezColor::White;

      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillSortingKey();
    }

    bool bDontCacheYet = false;

    // Determine render data category.
    ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;

    if (hMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowLoadingFallback);

      if (pMaterial.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
        bDontCacheYet = true;

      category = pMaterial->GetRenderDataCategory();
    }

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

void ezHeightfieldComponent::OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const
{
  if (!m_bGenerateCollision)
    return;

  ezGeometry geom;
  BuildGeometry(geom);

  auto* pDesc = msg.m_pStaticMeshDescription;
  auto& subMesh = pDesc->m_SubMeshes.ExpandAndGetRef();
  subMesh.m_uiFirstTriangle = pDesc->m_Triangles.GetCount();

  const ezTransform trans = GetOwner()->GetGlobalTransform();

  const ezUInt32 uiTriOffset = pDesc->m_Vertices.GetCount();
  pDesc->m_Vertices.Reserve(uiTriOffset + geom.GetVertices().GetCount());

  ezUInt32 uiTrisNeeded = 0;
  for (const auto& polys : geom.GetPolygons())
  {
    uiTrisNeeded += polys.m_Vertices.GetCount() - 2;
  }

  pDesc->m_Triangles.Reserve(pDesc->m_Triangles.GetCount() + uiTrisNeeded);

  for (const auto& verts : geom.GetVertices())
  {
    pDesc->m_Vertices.PushBack(trans * verts.m_vPosition);
  }

  for (const auto& polys : geom.GetPolygons())
  {
    for (ezUInt32 t = 0; t < polys.m_Vertices.GetCount() - 2; ++t)
    {
      auto& tri = pDesc->m_Triangles.ExpandAndGetRef();
      tri.m_uiVertexIndices[0] = uiTriOffset + polys.m_Vertices[0];
      tri.m_uiVertexIndices[1] = uiTriOffset + polys.m_Vertices[t + 1];
      tri.m_uiVertexIndices[2] = uiTriOffset + polys.m_Vertices[t + 2];
    }
  }

  subMesh.m_uiNumTriangles = pDesc->m_Triangles.GetCount() - subMesh.m_uiFirstTriangle;

  ezMaterialResourceHandle hMaterial = m_hMaterial;
  if (!hMaterial.IsValid())
  {
    // Data/Base/Materials/Common/Pattern.ezMaterialAsset
    hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
  }

  if (hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pMaterial.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      const ezString surface = pMaterial->GetSurface().GetString();

      if (!surface.IsEmpty())
      {
        ezUInt32 idx = pDesc->m_Surfaces.IndexOf(surface);
        if (idx == ezInvalidIndex)
        {
          idx = pDesc->m_Surfaces.GetCount();
          pDesc->m_Surfaces.PushBack(surface);
        }

        subMesh.m_uiSurfaceIndex = static_cast<ezUInt16>(idx);
      }
    }
  }
}

void ezHeightfieldComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && (m_bGenerateCollision == false || GetOwner()->IsDynamic()))
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

    desc.SetBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(vPosOffset, vPosOffset + vSize)));

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
