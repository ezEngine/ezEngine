#pragma once

#include <KrautPlugin/KrautDeclarations.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>

struct ezMsgExtractGeometry;
struct ezMsgBuildStaticMesh;
struct ezResourceEvent;
class ezKrautRenderData;
typedef ezTypedResourceHandle<class ezKrautTreeResource> ezKrautTreeResourceHandle;

class EZ_KRAUTPLUGIN_DLL ezKrautTreeComponentManager : public ezComponentManager<class ezKrautTreeComponent, ezBlockStorageType::Compact>
{
public:
  typedef ezComponentManager<ezKrautTreeComponent, ezBlockStorageType::Compact> SUPER;

  ezKrautTreeComponentManager(ezWorld* pWorld)
      : SUPER(pWorld)
  {
  }

  void Update(const ezWorldModule::UpdateContext& context);
  void EnqueueUpdate(ezComponentHandle hComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  mutable ezMutex m_Mutex;
  ezDeque<ezComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class EZ_KRAUTPLUGIN_DLL ezKrautTreeComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezKrautTreeComponent, ezRenderComponent, ezKrautTreeComponentManager);

public:
  ezKrautTreeComponent();
  ~ezKrautTreeComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void Initialize() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent interface

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezKrautTreeComponent interface
public:

  // see ezKrautTreeComponent::GetLocalBounds for details
  static const int s_iLocalBoundsScale = 3;

  void OnExtractGeometry(ezMsgExtractGeometry& msg) const;
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const;
  
  // ************************************* PROPERTIES ***********************************
public:

  void SetKrautTreeFile(const char* szFile);
  const char* GetKrautTreeFile() const;

protected:

  // ************************************* FUNCTIONS *****************************

public:
  void SetKrautTree(const ezKrautTreeResourceHandle& hTree);
  EZ_ALWAYS_INLINE const ezKrautTreeResourceHandle& GetKrautTree() const { return m_hKrautTree; }

private:
  ezResult CreateGeometry(ezGeometry& geo, ezWorldGeoExtractionUtil::ExtractionMode mode) const;

  ezSharedPtr<ezKrautLodInfo> m_pLodInfo;
  ezKrautTreeResourceHandle m_hKrautTree;
};
