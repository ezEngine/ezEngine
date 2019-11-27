#include <GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGreyBoxShape, 1)
  EZ_ENUM_CONSTANTS(ezGreyBoxShape::Box, ezGreyBoxShape::RampX, ezGreyBoxShape::RampY, ezGreyBoxShape::Column)
  EZ_ENUM_CONSTANTS(ezGreyBoxShape::StairsX, ezGreyBoxShape::StairsY, ezGreyBoxShape::ArchX, ezGreyBoxShape::ArchY, ezGreyBoxShape::SpiralStairs)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezGreyBoxComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("Shape", ezGreyBoxShape, GetShape, SetShape),
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    EZ_ACCESSOR_PROPERTY("SizeNegX", GetSizeNegX, SetSizeNegX),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosX", GetSizePosX, SetSizePosX),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizeNegY", GetSizeNegY, SetSizeNegY),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosY", GetSizePosY, SetSizePosY),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizeNegZ", GetSizeNegZ, SetSizeNegZ),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosZ", GetSizePosZ, SetSizePosZ),//->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(3, 32)),
    EZ_ACCESSOR_PROPERTY("Curvature", GetCurvature, SetCurvature),
    EZ_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SlopedTop", GetSlopedTop, SetSlopedTop),
    EZ_ACCESSOR_PROPERTY("SlopedBottom", GetSlopedBottom, SetSlopedBottom),
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
    new ezNonUniformBoxManipulatorAttribute("SizeNegX", "SizePosX", "SizeNegY", "SizePosY", "SizeNegZ", "SizePosZ"),
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
}

ezResult ezGreyBoxComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  GenerateRenderMesh();

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezGreyBoxComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  GenerateRenderMesh();

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

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::IfStatic);
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
  m_Curvature = ezAngle::Degree(ezMath::RoundToMultiple(curvature.GetDegree(), 5.0f));
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

void ezGreyBoxComponent::OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const
{
  ezGeometry geom;
  BuildGeometry(geom);
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

        subMesh.m_uiSurfaceIndex = idx;
      }
    }
  }
}

void ezGreyBoxComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh ||
      msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
  {
    // do not include this for the collision mesh, if the proper tag is not set
    if (!GetOwner()->GetTags().IsSetByName("AutoColMesh"))
      return;
  }
  else
  {
    EZ_ASSERT_DEBUG(msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh, "Unknown geometry extraction mode");
  }


  const ezTransform transform = GetOwner()->GetGlobalTransform();

  ezGeometry geom;
  BuildGeometry(geom);
  geom.TriangulatePolygons();

  const ezUInt32 uiVertexIdxOffset = msg.m_pWorldGeometry->m_Vertices.GetCount();

  auto& vertices = geom.GetVertices();
  for (auto& v : vertices)
  {
    auto& vert = msg.m_pWorldGeometry->m_Vertices.ExpandAndGetRef();
    vert.m_vPosition = transform * v.m_vPosition;
  }

  auto& triangles = geom.GetPolygons();
  for (auto& t : triangles)
  {
    auto& tri = msg.m_pWorldGeometry->m_Triangles.ExpandAndGetRef();
    tri.m_uiVertexIndices[0] = uiVertexIdxOffset + t.m_Vertices[0];
    tri.m_uiVertexIndices[1] = uiVertexIdxOffset + t.m_Vertices[1];
    tri.m_uiVertexIndices[2] = uiVertexIdxOffset + t.m_Vertices[2];
  }
}

void ezGreyBoxComponent::InvalidateMesh()
{
  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();
    TriggerLocalBoundsUpdate();
  }
}

void ezGreyBoxComponent::BuildGeometry(ezGeometry& geom) const
{
  ezVec3 size;
  size.x = m_fSizeNegX + m_fSizePosX;
  size.y = m_fSizeNegY + m_fSizePosY;
  size.z = m_fSizeNegZ + m_fSizePosZ;

  ezVec3 offset(0);
  offset.x = (m_fSizePosX - m_fSizeNegX) * 0.5f;
  offset.y = (m_fSizePosY - m_fSizeNegY) * 0.5f;
  offset.z = (m_fSizePosZ - m_fSizeNegZ) * 0.5f;

  ezMat4 t, t2, t3;
  t.SetTranslationMatrix(offset);

  switch (m_Shape)
  {
    case ezGreyBoxShape::Box:
      geom.AddTexturedBox(size, m_Color, t);
      break;

    case ezGreyBoxShape::RampX:
      geom.AddTexturedRamp(size, m_Color, t);
      break;

    case ezGreyBoxShape::RampY:
      ezMath::Swap(size.x, size.y);
      t.SetRotationMatrixZ(ezAngle::Degree(-90.0f));
      t.SetTranslationVector(offset);
      geom.AddTexturedRamp(size, m_Color, t);
      break;

    case ezGreyBoxShape::Column:
      t.SetScalingFactors(size);
      geom.AddCylinder(0.5f, 0.5f, 0.5f, 0.5f, true, true, m_uiDetail, m_Color, t);
      break;

    case ezGreyBoxShape::StairsX:
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, m_Color, t);
      break;

    case ezGreyBoxShape::StairsY:
      ezMath::Swap(size.x, size.y);
      t.SetRotationMatrixZ(ezAngle::Degree(-90.0f));
      t.SetTranslationVector(offset);
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, m_Color, t);
      break;

    case ezGreyBoxShape::ArchX:
    {
      const float tmp = size.z;
      size.z = size.x;
      size.x = size.y;
      size.y = tmp;
      t.SetRotationMatrixY(ezAngle::Degree(-90));
      t2.SetRotationMatrixX(ezAngle::Degree(90));
      t = t2 * t;
      t.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, m_Color, t);
    }
    break;

    case ezGreyBoxShape::ArchY:
    {
      t.SetRotationMatrixY(ezAngle::Degree(-90));
      t2.SetRotationMatrixX(ezAngle::Degree(90));
      t3.SetRotationMatrixZ(ezAngle::Degree(90));
      ezMath::Swap(size.y, size.z);
      t = t3 * t2 * t;
      t.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, m_Color, t);
    }
    break;

    case ezGreyBoxShape::SpiralStairs:
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, true, m_bSlopedBottom, m_bSlopedTop, m_Color, t);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

void ezGreyBoxComponent::GenerateRenderMesh() const
{
  if (m_hMesh.IsValid())
    return;

  ezStringBuilder sResourceName;

  switch (m_Shape)
  {
    case ezGreyBoxShape::Box:
      sResourceName.Format(
        "Grey-Box:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case ezGreyBoxShape::RampX:
      sResourceName.Format(
        "Grey-RampX:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case ezGreyBoxShape::RampY:
      sResourceName.Format(
        "Grey-RampY:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case ezGreyBoxShape::Column:
      sResourceName.Format("Grey-Column:{0}-{1},{2}-{3},{4}-{5}-d{6}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ,
        m_fSizePosZ, m_uiDetail);
      break;

    case ezGreyBoxShape::StairsX:
      sResourceName.Format("Grey-StairsX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY,
        m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case ezGreyBoxShape::StairsY:
      sResourceName.Format("Grey-StairsY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY,
        m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case ezGreyBoxShape::ArchX:
      sResourceName.Format("Grey-ArchX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY,
        m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case ezGreyBoxShape::ArchY:
      sResourceName.Format("Grey-ArchY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY,
        m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case ezGreyBoxShape::SpiralStairs:
      sResourceName.Format("Grey-Spiral:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}-st{9}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY,
        m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness, m_bSlopedTop);
      sResourceName.AppendFormat("-sb{0}", m_bSlopedBottom);
      break;


    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  ezGeometry geom;
  BuildGeometry(geom);

  geom.ComputeFaceNormals();
  geom.TriangulatePolygons();
  geom.ComputeTangents();

  ezMeshResourceDescriptor desc;

  // Data/Base/Materials/Common/Pattern.ezMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResourceName, std::move(desc), sResourceName);
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_GreyBoxComponent);
