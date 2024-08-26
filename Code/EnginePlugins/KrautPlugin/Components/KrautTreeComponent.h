#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
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

/// \brief Instantiates a Kraut tree model.
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

  /// \brief Currently this adds a cylinder mesh as a rough approximation for the tree collision shape.
  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const;
  /// \brief Currently this adds a cylinder mesh as a rough approximation for the tree collision shape.
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& ref_msg) const;

  /// \brief If non-default, this chooses a random variation of the tree.
  ///
  /// In the tree editor, designers can add "good seeds", ie seed values that produce nice results.
  /// Using the variation index you can select one of those good seeds.
  ///
  /// VariationIndex and CustomRandomSeed are mutually exclusive.
  /// If neither is set, a random variation is used, using the owner object's stable random seed.
  /// This is the preferred method to place trees and get a good random set, but requires that a tree model has defined "good seeds".
  void SetVariationIndex(ezUInt16 uiIndex); // [ property ]
  ezUInt16 GetVariationIndex() const;       // [ property ]

  /// \brief Trees with the same random seed look identical, different seeds produce different trees.
  ///
  /// \see SetVariationIndex()
  void SetCustomRandomSeed(ezUInt16 uiSeed); // [ property ]
  ezUInt16 GetCustomRandomSeed() const;      // [ property ]

  /// \brief Sets the Kraut resource that is used to generate the tree mesh.
  void SetKrautGeneratorResource(const ezKrautGeneratorResourceHandle& hTree);                          // [ property ]
  const ezKrautGeneratorResourceHandle& GetKrautGeneratorResource() const { return m_hKrautGenerator; } // [ property ]

private:
  /// \brief Currently this adds a cylinder mesh as a rough approximation of the tree trunk for collision.
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
