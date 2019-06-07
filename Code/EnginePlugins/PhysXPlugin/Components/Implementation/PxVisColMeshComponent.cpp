#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/Components/PxVisColMeshComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxVisColMeshComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh;Collision Mesh (Convex)")),
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxVisColMeshComponent::ezPxVisColMeshComponent() {}

ezPxVisColMeshComponent::~ezPxVisColMeshComponent() {}

void ezPxVisColMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
}


void ezPxVisColMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;

  GetWorld()->GetOrCreateComponentManager<ezPxVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

ezResult ezPxVisColMeshComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  // have to assume this isn't thread safe
  // CreateCollisionRenderMesh();

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezPxVisColMeshComponent::SetMeshFile(const char* szFile)
{
  ezPxMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezPxMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* ezPxVisColMeshComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void ezPxVisColMeshComponent::SetMesh(const ezPxMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh != hMesh)
  {
    m_hCollisionMesh = hMesh;
    m_hMesh.Invalidate();

    GetWorld()->GetOrCreateComponentManager<ezPxVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void ezPxVisColMeshComponent::CreateCollisionRenderMesh()
{
  if (!m_hCollisionMesh.IsValid())
  {
    ezPxStaticActorComponent* pSibling;
    if (!GetOwner()->TryGetComponentOfBaseType(pSibling))
      return;

    m_hCollisionMesh = pSibling->GetMesh();
  }

  if (!m_hCollisionMesh.IsValid())
    return;

  ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::NoFallback);

  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
    return;

  ezStringBuilder sColMeshName = pMesh->GetResourceID();
  sColMeshName.AppendFormat("_{0}_VisColMesh",
                            pMesh->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sColMeshName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }

  ezMeshResourceDescriptor md;
  auto& buffer = md.MeshBufferDesc();

  if (pMesh->GetConvexMesh() != nullptr)
  {
    auto pConvex = pMesh->GetConvexMesh();

    ezUInt32 uiNumTriangles = 0;
    for (ezUInt32 p = 0; p < pConvex->getNbPolygons(); ++p)
    {
      physx::PxHullPolygon poly;
      pConvex->getPolygonData(p, poly);

      uiNumTriangles += poly.mNbVerts - 2;
    }

    const auto pIndices = pConvex->getIndexBuffer();

    buffer.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    buffer.AllocateStreams(pConvex->getNbVertices(), ezGALPrimitiveTopology::Triangles, uiNumTriangles);

    for (ezUInt32 v = 0; v < pConvex->getNbVertices(); ++v)
    {
      buffer.SetVertexData<PxVec3>(0, v, pConvex->getVertices()[v]);
    }

    uiNumTriangles = 0;
    for (ezUInt32 p = 0; p < pConvex->getNbPolygons(); ++p)
    {
      physx::PxHullPolygon poly;
      pConvex->getPolygonData(p, poly);

      const auto pLocalIdx = &pIndices[poly.mIndexBase];

      for (ezUInt32 tri = 2; tri < poly.mNbVerts; ++tri)
      {
        buffer.SetTriangleIndices(uiNumTriangles, pLocalIdx[0], pLocalIdx[tri - 1], pLocalIdx[tri]);
        ++uiNumTriangles;
      }
    }

    md.AddSubMesh(uiNumTriangles, 0, 0);
  }
  else if (pMesh->GetTriangleMesh() != nullptr)
  {
    auto pTriMesh = pMesh->GetTriangleMesh();

    buffer.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    buffer.AllocateStreams(pTriMesh->getNbVertices(), ezGALPrimitiveTopology::Triangles, pTriMesh->getNbTriangles());

    for (ezUInt32 vtx = 0; vtx < pTriMesh->getNbVertices(); ++vtx)
    {
      buffer.SetVertexData<PxVec3>(0, vtx, pTriMesh->getVertices()[vtx]);
    }

    if (pTriMesh->getTriangleMeshFlags().isSet(PxTriangleMeshFlag::e16_BIT_INDICES))
    {
      const ezUInt16* pIndices = reinterpret_cast<const ezUInt16*>(pTriMesh->getTriangles());

      for (ezUInt32 tri = 0; tri < pTriMesh->getNbTriangles(); ++tri)
      {
        buffer.SetTriangleIndices(tri, pIndices[tri * 3 + 0], pIndices[tri * 3 + 1], pIndices[tri * 3 + 2]);
      }
    }
    else
    {
      const ezUInt32* pIndices = reinterpret_cast<const ezUInt32*>(pTriMesh->getTriangles());

      for (ezUInt32 tri = 0; tri < pTriMesh->getNbTriangles(); ++tri)
      {
        buffer.SetTriangleIndices(tri, pIndices[tri * 3 + 0], pIndices[tri * 3 + 1], pIndices[tri * 3 + 2]);
      }
    }

    md.AddSubMesh(pTriMesh->getNbTriangles(), 0, 0);
  }
  else
  {
    return;
  }

  md.ComputeBounds();

  md.SetMaterial(0, "Materials/Common/ColMesh.ezMaterial");

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sColMeshName, std::move(md), "Collision Mesh Visualization");

  TriggerLocalBoundsUpdate();
}


void ezPxVisColMeshComponent::Initialize()
{
  SUPER::Initialize();

  GetWorld()->GetOrCreateComponentManager<ezPxVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

void ezPxVisColMeshComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::IfStatic);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezPxVisColMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  ezWorldModule::UpdateFunctionDesc desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPxVisColMeshComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezPxVisColMeshComponentManager::ResourceEventHandler, this));
}

void ezPxVisColMeshComponentManager::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezPxVisColMeshComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezPxVisColMeshComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (const auto& hComp : m_RequireUpdate)
  {
    ezPxVisColMeshComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->CreateCollisionRenderMesh();
  }

  m_RequireUpdate.Clear();
}

void ezPxVisColMeshComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void ezPxVisColMeshComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if ((e.m_Type == ezResourceEvent::Type::ResourceContentUnloading || e.m_Type == ezResourceEvent::Type::ResourceContentUpdated) &&
      e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezPxMeshResource>())
  {
    EZ_LOCK(m_Mutex);

    ezPxMeshResourceHandle hResource((ezPxMeshResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const ezPxVisColMeshComponent* pComponent = static_cast<ezPxVisColMeshComponent*>(it.Value());

      if (pComponent->GetMesh() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}
