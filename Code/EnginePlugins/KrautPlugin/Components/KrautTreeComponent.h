#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <KrautPlugin/KrautDeclarations.h>
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

/// Component manager for ezKrautTreeComponent.
///
/// Drives per-frame LOD updates and wind simulation for all active tree components.
class EZ_KRAUTPLUGIN_DLL ezKrautTreeComponentManager : public ezComponentManager<class ezKrautTreeComponent, ezBlockStorageType::Compact>
{
public:
  using SUPER = ezComponentManager<ezKrautTreeComponent, ezBlockStorageType::Compact>;

  ezKrautTreeComponentManager(ezWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const ezWorldModule::UpdateContext& context);
  void UpdateWind(const ezWorldModule::UpdateContext& context);
  void EnqueueUpdate(ezComponentHandle hComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  mutable ezMutex m_Mutex;
  ezDeque<ezComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

/// Instantiates a Kraut tree model.
///
/// References an ezKrautGeneratorResource and selects a random seed to determine the tree's
/// visual variation. The component requests LOD meshes on demand via the generator resource
/// and renders the tree using the appropriate LOD for the current camera distance.
///
/// Seed selection priority (highest to lowest):
///   1. CustomRandomSeed — if set, always uses this exact seed.
///   2. VariationIndex — selects from the generator's curated "good seeds" list.
///   3. Owner object's stable random seed — used when neither override is set.
///
/// The local bounds are scaled by s_iLocalBoundsScale to give the renderer early visibility
/// even when only a rough bounding box is available before full mesh generation.
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

  /// Currently this adds a cylinder mesh as a rough approximation for the tree collision shape.
  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const;
  /// Currently this adds a cylinder mesh as a rough approximation for the tree collision shape.
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& ref_msg) const;

  /// Selects a variation from the generator resource's curated "good seeds" list.
  ///
  /// In the tree editor, designers can add "good seeds", i.e. seed values that produce nice results.
  /// Using the variation index you can select one of those good seeds.
  ///
  /// VariationIndex and CustomRandomSeed are mutually exclusive.
  /// If neither is set, a random variation is used, using the owner object's stable random seed.
  /// This is the preferred method to place trees and get a good random set, but requires that a tree model has defined "good seeds".
  void SetVariationIndex(ezUInt16 uiIndex); // [ property ]
  ezUInt16 GetVariationIndex() const;       // [ property ]

  /// Overrides the seed used for tree generation with a fixed value.
  ///
  /// Trees with the same random seed look identical; different seeds produce different trees.
  /// Mutually exclusive with SetVariationIndex().
  void SetCustomRandomSeed(ezUInt16 uiSeed); // [ property ]
  ezUInt16 GetCustomRandomSeed() const;      // [ property ]

  /// Sets the Kraut resource that is used to generate the tree mesh.
  void SetKrautGeneratorResource(const ezKrautGeneratorResourceHandle& hTree);                          // [ property ]
  const ezKrautGeneratorResourceHandle& GetKrautGeneratorResource() const { return m_hKrautGenerator; } // [ property ]

  // Development options for the Kraut asset preview
  ezInt8 m_iLodOverride = -1;             ///< When >= 0, forces a specific LOD index regardless of camera distance. -1 = automatic.
  bool m_bHideFrondsAndLeafs = false;     ///< When true, frond and leaf sub-meshes are skipped during rendering.
  bool m_bForceGenerateImmediate = false; ///< When true, LOD generation runs synchronously instead of via background tasks.

  const ezKrautTreeResourceHandle& GetKrautTreeResource() const { return m_hKrautTree; }

private:
  /// Currently this adds a cylinder mesh as a rough approximation of the tree trunk for collision.
  ezResult CreateGeometry(ezGeometry& geo, ezWorldGeoExtractionUtil::ExtractionMode mode) const;
  void EnsureTreeIsGenerated();

  ezUInt16 m_uiVariationIndex = 0xFFFF;
  ezUInt16 m_uiCustomRandomSeed = 0xFFFF;
  ezUInt32 m_uiCurrentSeed = 0;

  /// The LOD index rendered in the most recent frame, or -1 if nothing has been rendered yet.
  /// Used by EnsureTreeIsGenerated() to delay switching to a regenerated tree until that LOD is ready,
  /// so the old tree continues to render without flickering.
  mutable ezInt8 m_iLastRenderedLod = -1;

  ezKrautTreeResourceHandle m_hKrautTree;
  ezKrautGeneratorResourceHandle m_hKrautGenerator;

  void ComputeWind();

  ezVec3 m_vWindSpringPos;
  ezVec3 m_vWindSpringVel;

  mutable ezInstanceDataOffset m_InstanceDataOffset;
};
