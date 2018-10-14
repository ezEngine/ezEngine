#include <PCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <KrautPlugin/KrautTreeComponent.h>
#include <KrautPlugin/KrautTreeResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezKrautTreeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("KrautTree", GetKrautTreeFile, SetKrautTreeFile)->AddAttributes(new ezAssetBrowserAttribute("Kraut Tree")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Vegetation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeComponent::ezKrautTreeComponent() = default;

ezKrautTreeComponent::~ezKrautTreeComponent() = default;

void ezKrautTreeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hKrautTree;
}


void ezKrautTreeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hKrautTree;

  GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

ezResult ezKrautTreeComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezKrautTreeComponent::SetKrautTreeFile(const char* szFile)
{
  ezKrautTreeResourceHandle hTree;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hTree = ezResourceManager::LoadResource<ezKrautTreeResource>(szFile);
  }

  SetKrautTree(hTree);
}

const char* ezKrautTreeComponent::GetKrautTreeFile() const
{
  if (!m_hKrautTree.IsValid())
    return "";

  return m_hKrautTree.GetResourceID();
}

void ezKrautTreeComponent::SetKrautTree(const ezKrautTreeResourceHandle& hTree)
{
  if (m_hKrautTree != hTree)
  {
    m_hKrautTree = hTree;

    GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

ezMeshRenderData* ezKrautTreeComponent::CreateRenderData(ezUInt32 uiBatchId) const
{
  return ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
}

void ezKrautTreeComponent::Initialize()
{
  SUPER::Initialize();

  GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

void ezKrautTreeComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial;

    hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;
    const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, uiPartIndex, uiFlipWinding};
    ezUInt32 uiBatchId = ezHashing::xxHash32(data, sizeof(data));

    ezMeshRenderData* pRenderData = CreateRenderData(uiBatchId);
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_uiPartIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);
    }

    // Sort by material and then by mesh
    ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;
    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, uiSortingKey);
  }
}

void ezKrautTreeComponent::CreateKrautRenderMesh()
{
  if (!m_hKrautTree.IsValid())
    return;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::NoFallback);

  if (pTree->IsMissingResource())
    return;

  ezStringBuilder sTreeName = pTree->GetResourceID();
  sTreeName.AppendFormat("_{0}_TreeMesh",
                         pTree->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sTreeName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }

  ezMeshResourceDescriptor md;
  auto& buffer = md.MeshBufferDesc();

  {
    const auto& desc = pTree->m_Descriptor;

    ezUInt32 uiNumTriangles = desc.m_TriangleIndices.GetCount() / 3;

    buffer.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    buffer.AllocateStreams(desc.m_Positions.GetCount(), ezGALPrimitiveTopology::Triangles, uiNumTriangles);

    for (ezUInt32 v = 0; v < desc.m_Positions.GetCount(); ++v)
    {
      buffer.SetVertexData<ezVec3>(0, v, desc.m_Positions[v]);
    }

    for (ezUInt32 p = 0; p < uiNumTriangles; ++p)
    {
      buffer.SetTriangleIndices(p, desc.m_TriangleIndices[p * 3 + 0], desc.m_TriangleIndices[p * 3 + 1], desc.m_TriangleIndices[p * 3 + 2]);
    }

    md.AddSubMesh(uiNumTriangles, 0, 0);
  }

  md.ComputeBounds();

  md.SetMaterial(0, "Materials/Common/ColMesh.ezMaterial");

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sTreeName, md, "Kraut Tree Visualization");

  TriggerLocalBoundsUpdate();
}

//////////////////////////////////////////////////////////////////////////

void ezKrautTreeComponentManager::Initialize()
{
  SUPER::Initialize();

  ezWorldModule::UpdateFunctionDesc desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezKrautTreeComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezKrautTreeComponentManager::ResourceEventHandler, this));
}

void ezKrautTreeComponentManager::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezKrautTreeComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezKrautTreeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (const auto& hComp : m_RequireUpdate)
  {
    ezKrautTreeComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->CreateKrautRenderMesh();
  }

  m_RequireUpdate.Clear();
}

void ezKrautTreeComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void ezKrautTreeComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if ((e.m_EventType == ezResourceEventType::ResourceContentUnloading || e.m_EventType == ezResourceEventType::ResourceContentUpdated) &&
      e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezKrautTreeResource>())
  {
    EZ_LOCK(m_Mutex);

    ezKrautTreeResourceHandle hResource((ezKrautTreeResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const ezKrautTreeComponent* pComponent = static_cast<ezKrautTreeComponent*>(it.Value());

      if (pComponent->GetKrautTree() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}
