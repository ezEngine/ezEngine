#pragma once

#include <KrautPlugin/Basics.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <Core/ResourceManager/ResourceHandle.h>

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

protected:
  virtual ezKrautRenderData* CreateBranchRenderData(ezUInt32 uiBatchId) const;

  //////////////////////////////////////////////////////////////////////////
  // ezKrautTreeComponent interface
public:

  
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
  ezSharedPtr<ezKrautLodInfo> m_pLodInfo;
  ezKrautTreeResourceHandle m_hKrautTree;
};
