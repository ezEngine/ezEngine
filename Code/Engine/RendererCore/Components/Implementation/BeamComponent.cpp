#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <RendererCore/Components/BeamComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>


// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBeamComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TargetObject", DummyGetter, SetTargetObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material"), new ezRequiredAttribute()),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::White), new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new ezDefaultValueAttribute(ezVec4(0, 1, 0, 1))),
    EZ_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.001f, ezVariant()), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("MaxLength", GetMaxLength, SetMaxLength)->AddAttributes(new ezSuffixAttribute(" m")),
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
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
    EZ_MESSAGE_HANDLER(ezMsgSetCustomData, OnMsgSetCustomData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Below this length (in world units) the beam is not drawn.
  constexpr float s_fMinBeamLength = 0.001f;

  /// How far (in the owner's local space) the end point must move until the bounds get updated.
  constexpr float s_fBoundsUpdateEpsilon = 0.01f;

  /// Creates (once) the mesh that all beams share: a cross of two double-sided quads, from x = 0 to x = 1, 1 unit wide.
  ezMeshResourceHandle GetOrCreateUnitBeamMesh()
  {
    constexpr ezStringView sMeshName = "ezBeamComponent_UnitCross.createdAtRuntime.ezBinMesh";

    ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sMeshName);
    if (hMesh.IsValid())
      return hMesh;

    // The two quads go diagonally through the YZ plane:
    //
    //  3        1
    //
    //      x
    //
    //  4        2
    const float h = ezMath::Sqrt(0.5f) * 0.5f; // the corners are 0.5 away from the center
    const ezVec3 vCorner1(0, h, h);
    const ezVec3 vCorner2(0, h, -h);
    const ezVec3 vCorner3(0, -h, h);
    const ezVec3 vCorner4(0, -h, -h);
    const ezVec3 vEnd = ezVec3::MakeAxisX();

    ezGeometry g;

    // Each quad is double sided. The back side needs its own vertices with a flipped normal,
    // otherwise the normal can't be perpendicular to the surface on both sides.
    // The normal is perpendicular to X, so the tangent (which follows U along the beam) never becomes parallel to it.
    auto AddQuad = [&](const ezVec3& vCornerA, const ezVec3& vCornerB)
    {
      const ezVec3 p0 = vCornerA;
      const ezVec3 p1 = vCornerB;
      const ezVec3 p2 = vEnd + vCornerA;
      const ezVec3 p3 = vEnd + vCornerB;

      ezVec3 vNormal;
      vNormal.CalculateNormal(p0, p2, p3).AssertSuccess();

      for (ezUInt32 uiSide = 0; uiSide < 2; ++uiSide)
      {
        const bool bBackSide = uiSide == 1;
        const ezVec3 vSideNormal = bBackSide ? -vNormal : vNormal;

        ezUInt32 index0 = g.AddVertex(p0, vSideNormal, ezVec2(0, 0), ezColor::White);
        ezUInt32 index1 = g.AddVertex(p1, vSideNormal, ezVec2(0, 1), ezColor::White);
        ezUInt32 index2 = g.AddVertex(p2, vSideNormal, ezVec2(1, 0), ezColor::White);
        ezUInt32 index3 = g.AddVertex(p3, vSideNormal, ezVec2(1, 1), ezColor::White);

        ezUInt32 indices[] = {index0, index2, index3, index1};
        g.AddPolygon(ezArrayPtr(indices), bBackSide);
      }
    };

    AddQuad(vCorner1, vCorner4);
    AddQuad(vCorner2, vCorner3);

    g.ComputeTangents();

    ezMeshResourceDescriptor desc;
    auto& meshBufferDesc = desc.MeshBufferDesc();
    meshBufferDesc.AddCommonStreams();
    meshBufferDesc.AllocateStreamsFromGeometry(g, ezGALPrimitiveTopology::Triangles);
    desc.AddSubMesh(meshBufferDesc.GetPrimitiveCount(), 0, 0);
    desc.ComputeBounds();

    return ezResourceManager::GetOrCreateResource<ezMeshResource>(sMeshName, std::move(desc), "Unit beam mesh");
  }
} // namespace

ezBeamComponent::ezBeamComponent() = default;
ezBeamComponent::~ezBeamComponent() = default;

void ezBeamComponent::Update()
{
  ezVec3 vLocalEnd = ezVec3::MakeZero();

  ezVec3 vEnd;
  if (ComputeEndPosition(vEnd))
  {
    vLocalEnd = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(vEnd);
  }

  // Only the bounds depend on the end point. The geometry is stretched at render time.
  if ((vLocalEnd - m_vLastLocalEndPosition).GetLengthSquared() > ezMath::Square(s_fBoundsUpdateEpsilon))
  {
    m_vLastLocalEndPosition = vLocalEnd;
    GetOwner()->UpdateLocalBounds();
  }
}

bool ezBeamComponent::ComputeEndPosition(ezVec3& out_vEndPosition) const
{
  const ezVec3 vStart = GetOwner()->GetGlobalPosition();

  if (m_hTargetObject.IsInvalidated())
  {
    // no target set: go along the owner's X axis, MaxLength long (negative goes along -X)
    if (ezMath::Abs(m_fMaxLength) < s_fMinBeamLength)
      return false;

    out_vEndPosition = vStart + (GetOwner()->GetGlobalRotation() * ezVec3::MakeAxisX()) * m_fMaxLength;
    return true;
  }

  // a target was set, but it is gone or inactive
  const ezGameObject* pTargetObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hTargetObject, pTargetObject) || !pTargetObject->IsActive())
    return false;

  const ezVec3 vToTarget = pTargetObject->GetGlobalPosition() - vStart;
  const float fLength = vToTarget.GetLength();

  if (fLength < s_fMinBeamLength)
    return false;

  // with a target, the sign doesn't matter
  const float fMaxLength = ezMath::Abs(m_fMaxLength);

  if (fMaxLength > 0.0f && fLength > fMaxLength)
  {
    out_vEndPosition = vStart + vToTarget * (fMaxLength / fLength);
  }
  else
  {
    out_vEndPosition = vStart + vToTarget;
  }

  return true;
}

void ezBeamComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  inout_stream.WriteGameObjectHandle(m_hTargetObject);

  s << m_fWidth;
  s << m_fMaxLength;
  s << m_hMaterial;
  s << m_Color;
  s << m_vCustomData;
}

void ezBeamComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();
  m_hTargetObject = inout_stream.ReadGameObjectHandle();

  s >> m_fWidth;

  if (uiVersion >= 2)
  {
    s >> m_fMaxLength;
  }
  else
  {
    float fUVUnitsPerWorldUnit = 1.0f; // removed, the texture coordinates are normalized now
    s >> fUVUnitsPerWorldUnit;
  }

  s >> m_hMaterial;
  s >> m_Color;

  if (uiVersion >= 2)
  {
    s >> m_vCustomData;
  }
}

ezResult ezBeamComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ezVec3 vEnd;
  if (!ComputeEndPosition(vEnd))
    return EZ_FAILURE;

  const ezVec3 vLocalEnd = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(vEnd);

  ezVec3 pts[] = {ezVec3::MakeZero(), vLocalEnd};
  ezBoundingBox box = ezBoundingBox::MakeFromPoints(pts, 2);

  // The beam can point in any direction, so pad all axes.
  box.Grow(ezVec3(m_fWidth * 0.5f + s_fBoundsUpdateEpsilon));
  ref_bounds = ezBoundingBoxSphere::MakeFromBox(box);

  return EZ_SUCCESS;
}

void ezBeamComponent::OnActivated()
{
  SUPER::OnActivated();

  m_hMesh = GetOrCreateUnitBeamMesh();
}

void ezBeamComponent::OnDeactivated()
{
  ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();

  m_hMesh.Invalidate();
}

void ezBeamComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  ezVec3 vEnd;
  if (!ComputeEndPosition(vEnd))
    return;

  const ezTransform& ownerTransform = GetOwner()->GetGlobalTransform();
  const ezVec3 vDir = vEnd - ownerTransform.m_vPosition;
  const float fLength = vDir.GetLength();
  const ezVec3 vOwnerScale = ownerTransform.m_vScale.Abs();
  const float fWidth = m_fWidth * ezMath::Max(vOwnerScale.x, ezMath::Max(vOwnerScale.y, vOwnerScale.z));

  // Turn the owner's X axis toward the end point. This keeps the owner's roll, so rotating the owner around the beam rotates the cross.
  const ezVec3 vOwnerX = ownerTransform.m_qRotation * ezVec3::MakeAxisX();
  const ezVec3 vBeamDir = vDir / fLength;

  ezTransform beamTransform;
  beamTransform.m_vPosition = ownerTransform.m_vPosition;

  if (vOwnerX.Dot(vBeamDir) < -0.9999f)
  {
    // (nearly) opposite, e.g. a negative MaxLength without a target: turn around the owner's Z axis.
    // MakeShortestRotation() can't be used here, for nearly opposite directions it is numerically unstable.
    beamTransform.m_qRotation = ownerTransform.m_qRotation * ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisZ(), ezAngle::MakeFromDegree(180.0f));
  }
  else
  {
    beamTransform.m_qRotation = ezQuat::MakeShortestRotation(vOwnerX, vBeamDir) * ownerTransform.m_qRotation;
  }
  beamTransform.m_vScale.Set(fLength, fWidth, fWidth);

  // Force dynamic instance data buffer since the render data is not cached, so we would trash the static instance data buffer every frame.
  const bool bDynamic = true;
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, beamTransform, m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

  ezMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
  pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
  pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, m_hMaterial, m_hMesh);

  ezRenderData::Category category = ezMaterialResource::GetRenderDataCategory(m_hMaterial);

  msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
}

void ezBeamComponent::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

void ezBeamComponent::OnMsgSetCustomData(ezMsgSetCustomData& ref_msg)
{
  m_vCustomData = ref_msg.m_vData;
}

void ezBeamComponent::SetTargetObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hTargetObject = resolver(szReference, GetHandle(), "TargetObject");

  TriggerLocalBoundsUpdate();
}

void ezBeamComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;

  TriggerLocalBoundsUpdate();
}

float ezBeamComponent::GetWidth() const
{
  return m_fWidth;
}

void ezBeamComponent::SetMaxLength(float fMaxLength)
{
  m_fMaxLength = fMaxLength;

  TriggerLocalBoundsUpdate();
}

float ezBeamComponent::GetMaxLength() const
{
  return m_fMaxLength;
}

void ezBeamComponent::SetMaterial(const ezMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
}

const ezMaterialResourceHandle& ezBeamComponent::GetMaterial() const
{
  return m_hMaterial;
}

void ezBeamComponent::SetColor(const ezColor& color)
{
  m_Color = color;
}

void ezBeamComponent::SetCustomData(const ezVec4& vData)
{
  m_vCustomData = vData;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_BeamComponent);
