#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/OccluderComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

// TODO:
// * in editor, at startup the collider will be created multiple times, until all properties are set -> cache, do once
// * have a way to render the occluder when selected in editor ?

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOccluderType, 1)
  EZ_ENUM_CONSTANTS(ezOccluderType::Box, ezOccluderType::QuadPosX, ezOccluderType::Mesh)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezOccluderComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("Type", ezOccluderType, GetType, SetType),
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), {}), new ezDefaultValueAttribute(ezVec3(1.0f))),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::LightUI(ezColorScheme::Blue)),
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezOccluderComponentManager::ezOccluderComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezOccluderComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

ezOccluderComponent::ezOccluderComponent() = default;
ezOccluderComponent::~ezOccluderComponent() = default;

void ezOccluderComponent::SetExtents(const ezVec3& vExtents)
{
  if (m_vExtents == vExtents)
    return;

  m_vExtents = vExtents;

  if (m_Type != ezOccluderType::Mesh)
  {
    m_pOccluderObject.Clear();
    UpdateOccluder();
  }
}

void ezOccluderComponent::SetType(ezEnum<ezOccluderType> type)
{
  if (m_Type == type)
    return;

  m_Type = type;
  m_pOccluderObject.Clear();

  UpdateOccluder();
}

void ezOccluderComponent::SetMesh(const ezCpuMeshResourceHandle& hMesh)
{
  if (m_hMesh == hMesh)
    return;

  m_hMesh = hMesh;

  if (m_Type == ezOccluderType::Mesh)
  {
    m_pOccluderObject.Clear();
    UpdateOccluder();
  }
}

const ezCpuMeshResourceHandle& ezOccluderComponent::GetMesh() const
{
  return m_hMesh;
}

void ezOccluderComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  auto category = ezDefaultSpatialDataCategories::OcclusionDynamic;

  if (GetOwner()->IsStatic())
  {
    category = ezDefaultSpatialDataCategories::OcclusionStatic;
  }

  if (m_Type == ezOccluderType::Mesh)
  {
    if (m_pOccluderObject)
    {
      ezResourceLock<ezCpuMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pMesh.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        msg.AddBounds(pMesh->GetDescriptor().GetBounds(), category);
      }
    }
  }
  else
  {
    msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), category);
  }
}

void ezOccluderComponent::UpdateOccluder()
{
  if (!IsActiveAndInitialized())
    return;

  if (m_pOccluderObject != nullptr)
    return;

  switch (m_Type)
  {
    case ezOccluderType::Box:
      m_pOccluderObject = ezRasterizerObject::CreateBox(m_vExtents);
      break;

    case ezOccluderType::QuadPosX:
      m_pOccluderObject = ezRasterizerObject::CreateQuadX(ezVec2(m_vExtents.z, m_vExtents.y));
      break;

    case ezOccluderType::Mesh:
    {
      if (!m_hMesh.IsValid())
        return;

      ezResourceLock<ezCpuMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
        return;

      const auto& desc = pMesh->GetDescriptor().MeshBufferDesc();
      if (desc.GetTopology() != ezGALPrimitiveTopology::Triangles)
      {
        ezLog::Error("Mesh can't be used as a occluder, invalid topology: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      if (desc.GetVertexDeclaration().m_VertexStreams[0].m_Semantic != ezGALVertexAttributeSemantic::Position ||
          desc.GetVertexDeclaration().m_VertexStreams[0].m_uiOffset != 0)
      {
        ezLog::Error("Mesh can't be used as a occluder, invalid vertex stream configuration: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      if (!desc.HasIndexBuffer())
      {
        ezLog::Error("Mesh can't be used as a occluder, no index buffer: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      if (desc.Uses32BitIndices())
      {
        ezLog::Error("Mesh can't be used as a occluder, too many triangles: {}", m_hMesh.GetResourceIdOrDescription());
        return;
      }

      ezGeometry geo;

      const ezUInt32 uiStride = desc.GetVertexDataSize();
      const ezUInt8* pVtxData = desc.GetVertexBufferData().GetPtr();

      const ezUInt16* pIndices = (const ezUInt16*)desc.GetIndexBufferData().GetPtr();

      for (ezUInt32 vtx = 0; vtx < desc.GetVertexCount(); ++vtx)
      {
        const ezVec3 v = *(const ezVec3*)ezMemoryUtils::AddByteOffset(pVtxData, uiStride * vtx);

        geo.AddVertex(v, ezVec3(0, 0, 1));
      }

      ezUInt32 idx[3];

      for (ezUInt32 p = 0; p < desc.GetPrimitiveCount(); ++p)
      {
        idx[0] = pIndices[0];
        idx[1] = pIndices[1];
        idx[2] = pIndices[2];
        pIndices += 3;

        geo.AddPolygon(idx, false);
      }

      m_pOccluderObject = ezRasterizerObject::CreateMesh(pMesh->GetResourceID(), geo);

      break;
    }
  }

  GetOwner()->UpdateLocalBounds();
}

void ezOccluderComponent::OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const
{
  if (m_pOccluderObject == nullptr)
    return;

  switch (m_Type)
  {
    case ezOccluderType::Box:
      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
      break;

    case ezOccluderType::QuadPosX:
      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform() + GetOwner()->GetGlobalRotation() * ezVec3(m_vExtents.x * 0.5f, 0, 0));
      break;

    case ezOccluderType::Mesh:
      msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
      break;
  }
}

void ezOccluderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_Type;
  s << m_hMesh;
}

void ezOccluderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 2)
  {
    s >> m_Type;
  }

  if (uiVersion >= 3)
  {
    s >> m_hMesh;
  }
}

void ezOccluderComponent::OnActivated()
{
  m_pOccluderObject.Clear();

  UpdateOccluder();
}

void ezOccluderComponent::OnDeactivated()
{
  m_pOccluderObject.Clear();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_OccluderComponent);
