#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGreyBoxShape, 1)
  EZ_ENUM_CONSTANTS(ezGreyBoxShape::Box, ezGreyBoxShape::RampX, ezGreyBoxShape::RampY, ezGreyBoxShape::Column)
  EZ_ENUM_CONSTANTS(ezGreyBoxShape::StairsX, ezGreyBoxShape::StairsY, ezGreyBoxShape::ArchX, ezGreyBoxShape::ArchY, ezGreyBoxShape::SpiralStairs)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezGreyBoxComponent, 5, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("Shape", ezGreyBoxShape, GetShape, SetShape),
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White), new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("SizeNegX", GetSizeNegX, SetSizeNegX)->AddAttributes(new ezGroupAttribute("Size", "Size")),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosX", GetSizePosX, SetSizePosX),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizeNegY", GetSizeNegY, SetSizeNegY),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosY", GetSizePosY, SetSizePosY),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizeNegZ", GetSizeNegZ, SetSizeNegZ),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosZ", GetSizePosZ, SetSizePosZ),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new ezGroupAttribute("Misc"), new ezDefaultValueAttribute(16), new ezClampValueAttribute(3, 32)),
    EZ_ACCESSOR_PROPERTY("Curvature", GetCurvature, SetCurvature),
    EZ_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SlopedTop", GetSlopedTop, SetSlopedTop),
    EZ_ACCESSOR_PROPERTY("SlopedBottom", GetSlopedBottom, SetSlopedBottom),
    EZ_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("IncludeInNavmesh", GetIncludeInNavmesh, SetIncludeInNavmesh)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("UseAsOccluder", m_bUseAsOccluder)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
    new ezNonUniformBoxManipulatorAttribute("SizeNegX", "SizePosX", "SizeNegY", "SizePosY", "SizeNegZ", "SizePosZ"),
    new ezColorAttribute(ezColorScheme::Construction),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgBuildStaticMesh, OnBuildStaticMesh),
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
    EZ_MESSAGE_HANDLER(ezMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezGreyBoxComponent::ezGreyBoxComponent() = default;
ezGreyBoxComponent::~ezGreyBoxComponent() = default;

void ezGreyBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_Shape;
  s << m_hMaterial;
  s << m_fSizeNegX;
  s << m_fSizePosX;
  s << m_fSizeNegY;
  s << m_fSizePosY;
  s << m_fSizeNegZ;
  s << m_fSizePosZ;
  s << m_uiDetail;

  // Version 2
  s << m_Curvature;
  s << m_fThickness;
  s << m_bSlopedTop;
  s << m_bSlopedBottom;

  // Version 3
  s << m_Color;

  // Version 4
  s << m_bGenerateCollision;
  s << m_bIncludeInNavmesh;

  // Version 5
  s << m_bUseAsOccluder;
}

void ezGreyBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_Shape;
  s >> m_hMaterial;
  s >> m_fSizeNegX;
  s >> m_fSizePosX;
  s >> m_fSizeNegY;
  s >> m_fSizePosY;
  s >> m_fSizeNegZ;
  s >> m_fSizePosZ;
  s >> m_uiDetail;

  if (uiVersion >= 2)
  {
    s >> m_Curvature;
    s >> m_fThickness;
    s >> m_bSlopedTop;
    s >> m_bSlopedBottom;
  }

  if (uiVersion >= 3)
  {
    s >> m_Color;
  }

  if (uiVersion >= 4)
  {
    s >> m_bGenerateCollision;
    s >> m_bIncludeInNavmesh;
  }

  if (uiVersion >= 5)
  {
    s >> m_bUseAsOccluder;
  }
}

void ezGreyBoxComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<ezMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

ezResult ezGreyBoxComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();

    if (m_bUseAsOccluder)
    {
      msg.AddBounds(bounds, GetOwner()->IsStatic() ? ezDefaultSpatialDataCategories::OcclusionStatic : ezDefaultSpatialDataCategories::OcclusionDynamic);
    }

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezGreyBoxComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
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
      pRenderData->m_Color = m_Color;

      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
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

void ezGreyBoxComponent::SetShape(ezEnum<ezGreyBoxShape> shape)
{
  m_Shape = shape;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetMaterialFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* ezGreyBoxComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void ezGreyBoxComponent::SetSizeNegX(float f)
{
  m_fSizeNegX = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizePosX(float f)
{
  m_fSizePosX = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizeNegY(float f)
{
  m_fSizeNegY = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizePosY(float f)
{
  m_fSizePosY = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizeNegZ(float f)
{
  m_fSizeNegZ = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizePosZ(float f)
{
  m_fSizePosZ = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetDetail(ezUInt32 uiDetail)
{
  m_uiDetail = uiDetail;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetCurvature(ezAngle curvature)
{
  m_Curvature = ezAngle::MakeFromDegree(ezMath::RoundToMultiple(curvature.GetDegree(), 5.0f));
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSlopedTop(bool b)
{
  m_bSlopedTop = b;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSlopedBottom(bool b)
{
  m_bSlopedBottom = b;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetThickness(float f)
{
  m_fThickness = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void ezGreyBoxComponent::SetIncludeInNavmesh(bool b)
{
  m_bIncludeInNavmesh = b;
}

void ezGreyBoxComponent::OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const
{
  if (!m_bGenerateCollision)
    return;

  ezGeometry geom;
  BuildGeometry(geom, m_Shape, false);
  geom.TriangulatePolygons();

  auto* pDesc = msg.m_pStaticMeshDescription;
  auto& subMesh = pDesc->m_SubMeshes.ExpandAndGetRef();
  subMesh.m_uiFirstTriangle = pDesc->m_Triangles.GetCount();

  const ezTransform t = GetOwner()->GetGlobalTransform();

  const ezUInt32 uiTriOffset = pDesc->m_Vertices.GetCount();

  for (const auto& verts : geom.GetVertices())
  {
    pDesc->m_Vertices.PushBack(t * verts.m_vPosition);
  }

  for (const auto& polys : geom.GetPolygons())
  {
    auto& tri = pDesc->m_Triangles.ExpandAndGetRef();
    tri.m_uiVertexIndices[0] = uiTriOffset + polys.m_Vertices[0];
    tri.m_uiVertexIndices[1] = uiTriOffset + polys.m_Vertices[1];
    tri.m_uiVertexIndices[2] = uiTriOffset + polys.m_Vertices[2];
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

void ezGreyBoxComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && (m_bGenerateCollision == false || GetOwner()->IsDynamic()))
    return;

  if (msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && (m_bIncludeInNavmesh == false || GetOwner()->IsDynamic()))
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<ezCpuMeshResource>());
}

void ezGreyBoxComponent::OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const
{
  if (!IsActiveAndInitialized() || !m_bUseAsOccluder)
    return;

  if (m_pOccluderObject == nullptr)
  {
    ezEnum<ezGreyBoxShape> shape = m_Shape;
    if (shape == ezGreyBoxShape::StairsX && m_Curvature == ezAngle())
      shape = ezGreyBoxShape::RampX;
    if (shape == ezGreyBoxShape::StairsY && m_Curvature == ezAngle())
      shape = ezGreyBoxShape::RampY;

    ezStringBuilder sResourceName;
    GenerateMeshName(sResourceName);

    m_pOccluderObject = ezRasterizerObject::GetObject(sResourceName);

    if (m_pOccluderObject == nullptr)
    {
      ezGeometry geom;
      BuildGeometry(geom, shape, true);

      m_pOccluderObject = ezRasterizerObject::CreateMesh(sResourceName, geom);
    }
  }

  msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
}

void ezGreyBoxComponent::InvalidateMesh()
{
  m_pOccluderObject = nullptr;

  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<ezMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void ezGreyBoxComponent::BuildGeometry(ezGeometry& geom, ezEnum<ezGreyBoxShape> shape, bool bOnlyRoughDetails) const
{
  ezVec3 size;
  size.x = m_fSizeNegX + m_fSizePosX;
  size.y = m_fSizeNegY + m_fSizePosY;
  size.z = m_fSizeNegZ + m_fSizePosZ;

  ezVec3 offset(0);
  offset.x = (m_fSizePosX - m_fSizeNegX) * 0.5f;
  offset.y = (m_fSizePosY - m_fSizeNegY) * 0.5f;
  offset.z = (m_fSizePosZ - m_fSizeNegZ) * 0.5f;

  ezMat4 t2, t3;

  ezGeometry::GeoOptions opt;
  opt.m_Color = m_Color;
  opt.m_Transform = ezMat4::MakeTranslation(offset);

  switch (shape)
  {
    case ezGreyBoxShape::Box:
      geom.AddBox(size, true, opt);
      break;

    case ezGreyBoxShape::RampX:
      geom.AddTexturedRamp(size, opt);
      break;

    case ezGreyBoxShape::RampY:
      ezMath::Swap(size.x, size.y);
      opt.m_Transform = ezMat4::MakeRotationZ(ezAngle::MakeFromDegree(-90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddTexturedRamp(size, opt);
      break;

    case ezGreyBoxShape::Column:
      opt.m_Transform.SetScalingFactors(size).IgnoreResult();
      geom.AddCylinder(0.5f, 0.5f, 0.5f, 0.5f, true, true, static_cast<ezUInt16>(m_uiDetail), opt);
      break;

    case ezGreyBoxShape::StairsX:
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case ezGreyBoxShape::StairsY:
      ezMath::Swap(size.x, size.y);
      opt.m_Transform = ezMat4::MakeRotationZ(ezAngle::MakeFromDegree(-90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case ezGreyBoxShape::ArchX:
    {
      const float tmp = size.z;
      size.z = size.x;
      size.x = size.y;
      size.y = tmp;
      opt.m_Transform = ezMat4::MakeRotationY(ezAngle::MakeFromDegree(-90));
      t2 = ezMat4::MakeRotationX(ezAngle::MakeFromDegree(90));
      opt.m_Transform = t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case ezGreyBoxShape::ArchY:
    {
      opt.m_Transform = ezMat4::MakeRotationY(ezAngle::MakeFromDegree(-90));
      t2 = ezMat4::MakeRotationX(ezAngle::MakeFromDegree(90));
      t3 = ezMat4::MakeRotationZ(ezAngle::MakeFromDegree(90));
      ezMath::Swap(size.y, size.z);
      opt.m_Transform = t3 * t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case ezGreyBoxShape::SpiralStairs:
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, true, m_bSlopedBottom, m_bSlopedTop, true, opt);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

void ezGreyBoxComponent::GenerateMeshName(ezStringBuilder& out_sName) const
{
  switch (m_Shape)
  {
    case ezGreyBoxShape::Box:
      out_sName.Format("Grey-Box:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case ezGreyBoxShape::RampX:
      out_sName.Format("Grey-RampX:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case ezGreyBoxShape::RampY:
      out_sName.Format("Grey-RampY:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case ezGreyBoxShape::Column:
      out_sName.Format("Grey-Column:{0}-{1},{2}-{3},{4}-{5}-d{6}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail);
      break;

    case ezGreyBoxShape::StairsX:
      out_sName.Format("Grey-StairsX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case ezGreyBoxShape::StairsY:
      out_sName.Format("Grey-StairsY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case ezGreyBoxShape::ArchX:
      out_sName.Format("Grey-ArchX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case ezGreyBoxShape::ArchY:
      out_sName.Format("Grey-ArchY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case ezGreyBoxShape::SpiralStairs:
      out_sName.Format("Grey-Spiral:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}-st{9}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness, m_bSlopedTop);
      out_sName.AppendFormat("-sb{0}", m_bSlopedBottom);
      break;


    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

void ezGreyBoxComponent::GenerateMeshResourceDescriptor(ezMeshResourceDescriptor& desc) const
{
  ezGeometry geom;
  BuildGeometry(geom, m_Shape, false);

  bool bInvertedGeo = false;

  if (-m_fSizeNegX > m_fSizePosX)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegY > m_fSizePosY)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegZ > m_fSizePosZ)
    bInvertedGeo = !bInvertedGeo;

  if (bInvertedGeo)
  {
    for (auto vert : geom.GetVertices())
    {
      vert.m_vNormal = -vert.m_vNormal;
    }
  }

  geom.TriangulatePolygons();
  geom.ComputeTangents();

  // Data/Base/Materials/Common/Pattern.ezMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();
}

template <typename ResourceType>
ezTypedResourceHandle<ResourceType> ezGreyBoxComponent::GenerateMesh() const
{
  ezStringBuilder sResourceName;
  GenerateMeshName(sResourceName);

  ezTypedResourceHandle<ResourceType> hResource = ezResourceManager::GetExistingResource<ResourceType>(sResourceName);
  if (hResource.IsValid())
    return hResource;

  ezMeshResourceDescriptor desc;
  GenerateMeshResourceDescriptor(desc);

  return ezResourceManager::GetOrCreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GreyBoxComponent);
