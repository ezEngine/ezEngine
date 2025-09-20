#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <RendererCore/Components/BeamComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Device/Device.h>


// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBeamComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TargetObject", DummyGetter, SetTargetObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    EZ_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.001f, ezVariant()), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("UVUnitsPerWorldUnit", GetUVUnitsPerWorldUnit, SetUVUnitsPerWorldUnit)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBeamComponent::ezBeamComponent() = default;
ezBeamComponent::~ezBeamComponent() = default;

void ezBeamComponent::Update()
{
  ezGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    ezVec3 currentOwnerPosition = GetOwner()->GetGlobalPosition();
    ezVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();

    if (!pTargetObject->IsActive())
    {
      currentTargetPosition = currentOwnerPosition;
    }

    bool updateMesh = false;

    if ((currentOwnerPosition - m_vLastOwnerPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastOwnerPosition = currentOwnerPosition;
    }

    if ((currentTargetPosition - m_vLastTargetPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastTargetPosition = currentTargetPosition;
    }

    if (updateMesh)
    {
      ReinitMeshes();
    }
  }
  else
  {
    m_hMesh.Invalidate();
  }
}

void ezBeamComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  inout_stream.WriteGameObjectHandle(m_hTargetObject);

  s << m_fWidth;
  s << m_fUVUnitsPerWorldUnit;
  s << m_hMaterial;
  s << m_Color;
}

void ezBeamComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  m_hTargetObject = inout_stream.ReadGameObjectHandle();

  s >> m_fWidth;
  s >> m_fUVUnitsPerWorldUnit;
  s >> m_hMaterial;
  s >> m_Color;
}

ezResult ezBeamComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ezGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    const ezVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();
    const ezVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(currentTargetPosition);

    ezVec3 pts[] = {ezVec3::MakeZero(), targetPositionInOwnerSpace};

    ezBoundingBox box = ezBoundingBox::MakeFromPoints(pts, 2);
    const float fHalfWidth = m_fWidth * 0.5f;
    box.m_vMin -= ezVec3(0, fHalfWidth, fHalfWidth);
    box.m_vMax += ezVec3(0, fHalfWidth, fHalfWidth);
    ref_bounds = ezBoundingBoxSphere::MakeFromBox(box);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}


void ezBeamComponent::OnActivated()
{
  SUPER::OnActivated();

  ReinitMeshes();
}

void ezBeamComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  Cleanup();
}

void ezBeamComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_Color = m_Color;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillSortingKey();
  }

  // Determine render data category.
  ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
  ezRenderData::Category category = pMaterial->GetRenderDataCategory();

  msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
}

void ezBeamComponent::SetTargetObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hTargetObject = resolver(szReference, GetHandle(), "TargetObject");

  ReinitMeshes();
}

void ezBeamComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;

  ReinitMeshes();
}

float ezBeamComponent::GetWidth() const
{
  return m_fWidth;
}

void ezBeamComponent::SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit)
{
  if (fUVUnitsPerWorldUnit <= 0.0f)
    return;

  m_fUVUnitsPerWorldUnit = fUVUnitsPerWorldUnit;

  ReinitMeshes();
}

float ezBeamComponent::GetUVUnitsPerWorldUnit() const
{
  return m_fUVUnitsPerWorldUnit;
}

ezMaterialResourceHandle ezBeamComponent::GetMaterial() const
{
  return m_hMaterial;
}

void ezBeamComponent::CreateMeshes()
{
  ezVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(m_vLastTargetPosition);

  if (targetPositionInOwnerSpace.IsZero(0.01f))
    return;

  // Create the beam mesh name, it expresses the beam in local space with it's width
  // this way multiple beams in a corridor can share the same mesh for example.
  ezStringBuilder meshName;
  meshName.SetFormat("ezBeamComponent_{0}_{1}_{2}_{3}.createdAtRuntime.ezBinMesh", m_fWidth, ezArgF(targetPositionInOwnerSpace.x, 2), ezArgF(targetPositionInOwnerSpace.y, 2), ezArgF(targetPositionInOwnerSpace.z, 2));

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(meshName);

  // We build a cross mesh, thus we need the following vectors, x is the origin and we need to construct
  // the star points.
  //
  //  3        1
  //
  //      x
  //
  //  4        2
  ezVec3 crossVector1 = (0.5f * ezVec3::MakeAxisY() + 0.5f * ezVec3::MakeAxisZ());
  crossVector1.SetLength(m_fWidth * 0.5f).IgnoreResult();

  ezVec3 crossVector2 = (0.5f * ezVec3::MakeAxisY() - 0.5f * ezVec3::MakeAxisZ());
  crossVector2.SetLength(m_fWidth * 0.5f).IgnoreResult();

  ezVec3 crossVector3 = (-0.5f * ezVec3::MakeAxisY() + 0.5f * ezVec3::MakeAxisZ());
  crossVector3.SetLength(m_fWidth * 0.5f).IgnoreResult();

  ezVec3 crossVector4 = (-0.5f * ezVec3::MakeAxisY() - 0.5f * ezVec3::MakeAxisZ());
  crossVector4.SetLength(m_fWidth * 0.5f).IgnoreResult();

  const float fDistance = (m_vLastOwnerPosition - m_vLastTargetPosition).GetLength();



  // Build mesh if no existing one is found
  if (!m_hMesh.IsValid())
  {
    ezGeometry g;

    // Quad 1
    {
      ezUInt32 index0 = g.AddVertex(ezVec3::MakeZero() + crossVector1, ezVec3::MakeAxisX(), ezVec2(0, 0), ezColor::White);
      ezUInt32 index1 = g.AddVertex(ezVec3::MakeZero() + crossVector4, ezVec3::MakeAxisX(), ezVec2(0, 1), ezColor::White);
      ezUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector1, ezVec3::MakeAxisX(), ezVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), ezColor::White);
      ezUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector4, ezVec3::MakeAxisX(), ezVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), ezColor::White);

      ezUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(ezArrayPtr(indices), false);
      g.AddPolygon(ezArrayPtr(indices), true);
    }

    // Quad 2
    {
      ezUInt32 index0 = g.AddVertex(ezVec3::MakeZero() + crossVector2, ezVec3::MakeAxisX(), ezVec2(0, 0), ezColor::White);
      ezUInt32 index1 = g.AddVertex(ezVec3::MakeZero() + crossVector3, ezVec3::MakeAxisX(), ezVec2(0, 1), ezColor::White);
      ezUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector2, ezVec3::MakeAxisX(), ezVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), ezColor::White);
      ezUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector3, ezVec3::MakeAxisX(), ezVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), ezColor::White);

      ezUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(ezArrayPtr(indices), false);
      g.AddPolygon(ezArrayPtr(indices), true);
    }

    g.ComputeTangents();

    ezMeshResourceDescriptor desc;
    BuildMeshResourceFromGeometry(g, desc);

    m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(meshName, std::move(desc));
  }
}

void ezBeamComponent::BuildMeshResourceFromGeometry(ezGeometry& Geometry, ezMeshResourceDescriptor& MeshDesc) const
{
  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddCommonStreams();
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, ezGALPrimitiveTopology::Triangles);

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}

void ezBeamComponent::ReinitMeshes()
{
  Cleanup();

  if (IsActiveAndInitialized())
  {
    CreateMeshes();
    GetOwner()->UpdateLocalBounds();
  }
}

void ezBeamComponent::Cleanup()
{
  m_hMesh.Invalidate();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_BeamComponent);
