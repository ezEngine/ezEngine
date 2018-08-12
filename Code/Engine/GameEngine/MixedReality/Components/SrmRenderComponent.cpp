#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/MixedReality/Components/SrmRenderComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <WindowsMixedReality/SpatialMapping/SurfaceReconstructionMeshManager.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSrmRenderComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material"), new ezDefaultValueAttribute("Materials/Common/SRM_Visible.ezMaterial")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Mixed Reality"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSrmRenderComponent::ezSrmRenderComponent()
{
  SetMaterialFile("Materials/Common/SRM_Visible.ezMaterial");
}

ezSrmRenderComponent::~ezSrmRenderComponent() {}

void ezSrmRenderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_hMaterial;
}

void ezSrmRenderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hMaterial;
}

void ezSrmRenderComponent::OnActivated()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  ezSurfaceReconstructionMeshManager* pMan = ezSurfaceReconstructionMeshManager::GetSingleton();
  if (pMan == nullptr)
    return;

  pMan->m_Events.AddEventHandler(ezMakeDelegate(&ezSrmRenderComponent::SurfaceReconstructionManagerEventHandler, this));

  const auto& surfaces = pMan->BeginAccessingSurfaces();

  for (auto it = surfaces.GetIterator(); it.IsValid(); ++it)
  {
    UpdateSurfaceRepresentation(it.Key());
  }

  pMan->EndAccessingSurfaces();
#endif
}

void ezSrmRenderComponent::OnDeactivated()
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  ezSurfaceReconstructionMeshManager* pMan = ezSurfaceReconstructionMeshManager::GetSingleton();
  if (pMan == nullptr)
    return;

  if (pMan->m_Events.HasEventHandler(ezMakeDelegate(&ezSrmRenderComponent::SurfaceReconstructionManagerEventHandler, this)))
  {
    pMan->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSrmRenderComponent::SurfaceReconstructionManagerEventHandler, this));
  }

  while (!m_SrmRenderObjects.IsEmpty())
  {
    RemoveSrmRenderObject(m_SrmRenderObjects.GetIterator().Key());
  }

  m_SrmRenderObjects.Clear();
#endif
}

void ezSrmRenderComponent::SetMaterialFile(const char* szFile)
{
  ezMaterialResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }

  SetMaterial(hResource);
}

const char* ezSrmRenderComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void ezSrmRenderComponent::SetMaterial(const ezMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
}

void ezSrmRenderComponent::SurfaceReconstructionManagerEventHandler(const ezSrmManagerEvent& e)
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (e.m_Type == ezSrmManagerEvent::Type::MeshRemoved)
  {
    RemoveSrmRenderObject(e.m_MeshGuid);
    return;
  }

  if (e.m_Type == ezSrmManagerEvent::Type::MeshUpdated)
  {
    UpdateSurfaceRepresentation(e.m_MeshGuid);
  }
#endif
}

void ezSrmRenderComponent::RemoveSrmRenderObject(const ezUuid& guid)
{
  auto it = m_SrmRenderObjects.Find(guid);

  if (!it.IsValid())
    return;

  if (!it.Value().m_hGameObject.IsInvalidated())
  {
    GetWorld()->DeleteObjectDelayed(it.Value().m_hGameObject);
    it.Value().m_hGameObject.Invalidate();
  }

  m_SrmRenderObjects.Remove(it);
}

void ezSrmRenderComponent::UpdateSurfaceRepresentation(const ezUuid& guid)
{
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  ezSurfaceReconstructionMeshManager* pMan = ezSurfaceReconstructionMeshManager::GetSingleton();

  const auto& surfaces = pMan->BeginAccessingSurfaces();
  auto itSurf = surfaces.Find(guid);

  if (!itSurf.IsValid())
    goto end;

  const auto& surface = itSurf.Value();
  auto& ownSurface = m_SrmRenderObjects[guid];

  if (surface.m_iLastUpdate == 0) // removed
  {
    RemoveSrmRenderObject(guid);
    goto end;
  }

  if (ownSurface.m_iLastUpdate >= surface.m_iLastUpdate)
    goto end;

  ownSurface.m_iLastUpdate = surface.m_iLastUpdate;

  CreateSurfaceRepresentation(guid, ownSurface, surface.m_Transform, surface.m_MeshData);

end:
  pMan->EndAccessingSurfaces();
#endif
}

void ezSrmRenderComponent::CreateSurfaceRepresentation(const ezUuid& guid, SrmRenderObject& surface, const ezTransform& transform,
                                                       const ezMeshBufferResourceDescriptor& mb)
{
  EZ_LOCK(GetWorld()->GetWriteMarker());

  if (!surface.m_hGameObject.IsInvalidated())
  {
    GetWorld()->DeleteObjectDelayed(surface.m_hGameObject);
    surface.m_hGameObject.Invalidate();
  }

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(guid, sGuid);

  ezStringBuilder sName;
  sName.Format("SRMB_{0}_{1}", sGuid, surface.m_iLastUpdate);
  ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(sName, mb);

  ezMeshResourceDescriptor meshDesc;
  meshDesc.UseExistingMeshBuffer(hMeshBuffer);
  meshDesc.AddSubMesh(mb.GetPrimitiveCount(), 0, 0);
  meshDesc.SetMaterial(0, GetMaterialFile());
  meshDesc.ComputeBounds();

  sName.Format("SRM_{0}_{1}", sGuid, surface.m_iLastUpdate);
  ezMeshResourceHandle hMeshResource = ezResourceManager::CreateResource<ezMeshResource>(sName, meshDesc);

  ezGameObjectDesc obj;
  obj.m_hParent = GetOwner()->GetHandle();
  obj.m_LocalPosition = transform.m_vPosition;
  obj.m_LocalRotation = transform.m_qRotation;
  obj.m_LocalScaling = transform.m_vScale;

  ezGameObject* pObject;
  surface.m_hGameObject = GetWorld()->CreateObject(obj, pObject);

  ezMeshComponentManager* pMeshMan = GetWorld()->GetOrCreateComponentManager<ezMeshComponentManager>();

  ezMeshComponent* pMeshComp;
  surface.m_hMeshComponent = pMeshMan->CreateComponent(pObject, pMeshComp);

  pMeshComp->SetMesh(hMeshResource);
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_MixedReality_Components_SrmRenderComponent);
