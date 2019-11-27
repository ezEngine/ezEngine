#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void Initialize() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezKrautTreeComponent

public:
  ezKrautTreeComponent();
  ~ezKrautTreeComponent();

  // see ezKrautTreeComponent::GetLocalBounds for details
  static const int s_iLocalBoundsScale = 3;

  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const;

  void SetKrautTreeFile(const char* szFile); // [ property ]
  const char* GetKrautTreeFile() const;      // [ property ]

  void SetKrautTree(const ezKrautTreeResourceHandle& hTree);
  EZ_ALWAYS_INLINE const ezKrautTreeResourceHandle& GetKrautTree() const { return m_hKrautTree; }

private:
  ezResult CreateGeometry(ezGeometry& geo, ezWorldGeoExtractionUtil::ExtractionMode mode) const;

  ezSharedPtr<ezKrautLodInfo> m_pLodInfo;
  ezKrautTreeResourceHandle m_hKrautTree;
};
