#include <PCH.h>
#include <GameEngine/Components/GreyBoxComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGreyBoxShape, 1)
EZ_ENUM_CONSTANTS(ezGreyBoxShape::Box, ezGreyBoxShape::RampX, ezGreyBoxShape::RampY, ezGreyBoxShape::Column, ezGreyBoxShape::StairsX, ezGreyBoxShape::StairsY, ezGreyBoxShape::ArchX, ezGreyBoxShape::ArchY)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_COMPONENT_TYPE(ezGreyBoxComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("Shape", ezGreyBoxShape, GetShape, SetShape),
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("SizeNegX", GetSizeNegX, SetSizeNegX)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosX", GetSizePosX, SetSizePosX)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizeNegY", GetSizeNegY, SetSizeNegY)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosY", GetSizePosY, SetSizePosY)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizeNegZ", GetSizeNegZ, SetSizeNegZ)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizePosZ", GetSizePosZ, SetSizePosZ)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(3, 32)),
    EZ_ACCESSOR_PROPERTY("Curvature", GetCurvature, SetCurvature),
    EZ_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SlopedTop", GetSlopedTop, SetSlopedTop),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
  }
  EZ_END_ATTRIBUTES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
    EZ_MESSAGE_HANDLER(ezBuildStaticMeshMessage, OnBuildStaticMesh),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezGreyBoxComponent::ezGreyBoxComponent()
{
}

ezGreyBoxComponent::~ezGreyBoxComponent()
{
}

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
}

ezResult ezGreyBoxComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  GenerateRenderMesh();

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezGreyBoxComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  GenerateRenderMesh();

  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[uiMaterialIndex];

    const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = { uiMeshIDHash, uiMaterialIDHash, uiPartIndex, uiFlipWinding };
    ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;

      pRenderData->m_uiPartIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueID() | (uiMaterialIndex << 24);
    }

    // Determine render data category.
    ezRenderData::Category category;
    if (msg.m_OverrideCategory != ezInvalidIndex)
    {
      category = msg.m_OverrideCategory;
    }
    else
    {
      category = ezDefaultRenderDataCategories::LitOpaque;
    }

    // Sort by material and then by mesh
    ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;
    msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);
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
  m_Curvature = ezAngle::Degree(ezMath::Round(curvature.GetDegree(), 5.0f));
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSlopedTop(bool b)
{
  m_bSlopedTop = b;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetThickness(float f)
{
  m_fThickness = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::OnBuildStaticMesh(ezBuildStaticMeshMessage& msg) const
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

  if (m_hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::NoFallback);

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
    geom.AddTexturedBox(size, ezColor::White, t);
    break;

  case ezGreyBoxShape::RampX:
    geom.AddTexturedRamp(size, ezColor::White, t);
    break;

  case ezGreyBoxShape::RampY:
    ezMath::Swap(size.x, size.y);
    t.SetRotationMatrixZ(ezAngle::Degree(-90.0f));
    t.SetTranslationVector(offset);
    geom.AddTexturedRamp(size, ezColor::White, t);
    break;

  case ezGreyBoxShape::Column:
    t.SetScalingFactors(size);
    geom.AddCylinder(0.5f, 0.5f, 1.0f, true, true, m_uiDetail, ezColor::White, t);
    break;

  case ezGreyBoxShape::StairsX:
    geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, ezColor::White, t);
    break;

  case ezGreyBoxShape::StairsY:
    ezMath::Swap(size.x, size.y);
    t.SetRotationMatrixZ(ezAngle::Degree(-90.0f));
    t.SetTranslationVector(offset);
    geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, ezColor::White, t);
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
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, ezColor::White, t);
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
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, ezColor::White, t);
    }
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
    sResourceName.Format("Grey-Box:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
    break;

  case ezGreyBoxShape::RampX:
    sResourceName.Format("Grey-RampX:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
    break;

  case ezGreyBoxShape::RampY:
    sResourceName.Format("Grey-RampY:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
    break;

  case ezGreyBoxShape::Column:
    sResourceName.Format("Grey-Column:{0}-{1},{2}-{3},{4}-{5}-d{6}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail);
    break;

  case ezGreyBoxShape::StairsX:
    sResourceName.Format("Grey-StairsX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
    break;

  case ezGreyBoxShape::StairsY:
    sResourceName.Format("Grey-StairsY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
    break;

  case ezGreyBoxShape::ArchX:
    sResourceName.Format("Grey-ArchX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
    break;
  case ezGreyBoxShape::ArchY:
    sResourceName.Format("Grey-ArchY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
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

  // Data/Base/Materials/Prototyping/PrototypeGrey.ezMaterialAsset
  desc.SetMaterial(0, "{ 6bd5e7e6-b7be-9801-e032-14226cba1e96 }");

  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResourceName, desc, sResourceName);
}
