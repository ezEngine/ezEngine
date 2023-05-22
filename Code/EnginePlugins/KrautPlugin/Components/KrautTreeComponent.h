#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

struct ezMsgExtractGeometry;
struct ezMsgBuildStaticMesh;
struct ezResourceEvent;
class ezKrautRenderData;
class ezAbstractObjectNode;

using ezKrautTreeResourceHandle = ezTypedResourceHandle<class ezKrautTreeResource>;
using ezKrautGeneratorResourceHandle = ezTypedResourceHandle<class ezKrautGeneratorResource>;

class EZ_KRAUTPLUGIN_DLL ezKrautTreeComponentManager : public ezComponentManager<class ezKrautTreeComponent, ezBlockStorageType::Compact>
{
public:
  using SUPER = ezComponentManager<ezKrautTreeComponent, ezBlockStorageType::Compact>;

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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezKrautTreeComponent

public:
  ezKrautTreeComponent();
  ~ezKrautTreeComponent();

  // see ezKrautTreeComponent::GetLocalBounds for details
  static const int s_iLocalBoundsScale = 3;

  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const;
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& ref_msg) const;

  void SetKrautFile(const char* szFile); // [ property ]
  const char* GetKrautFile() const;      // [ property ]

  void SetVariationIndex(ezUInt16 uiIndex); // [ property ]
  ezUInt16 GetVariationIndex() const;       // [ property ]

  void SetCustomRandomSeed(ezUInt16 uiSeed); // [ property ]
  ezUInt16 GetCustomRandomSeed() const;      // [ property ]

  void SetKrautGeneratorResource(const ezKrautGeneratorResourceHandle& hTree);
  const ezKrautGeneratorResourceHandle& GetKrautGeneratorResource() const { return m_hKrautGenerator; }

private:
  ezResult CreateGeometry(ezGeometry& geo, ezWorldGeoExtractionUtil::ExtractionMode mode) const;
  void EnsureTreeIsGenerated();

  ezUInt16 m_uiVariationIndex = 0xFFFF;
  ezUInt16 m_uiCustomRandomSeed = 0xFFFF;
  ezKrautTreeResourceHandle m_hKrautTree;
  ezKrautGeneratorResourceHandle m_hKrautGenerator;

  void ComputeWind() const;

  mutable ezUInt64 m_uiLastWindUpdate = (ezUInt64)-1;
  mutable ezVec3 m_vWindSpringPos;
  mutable ezVec3 m_vWindSpringVel;
};
