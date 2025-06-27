#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Threading/TaskSystem.h>
#include <GameEngine/Physics/Breakable2D.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;

struct ezMsgPhysicsAddImpulse;
struct ezMsgExtractRenderData;
class ezGeometry;
class ezMeshResourceDescriptor;
class ezJoltMaterial;

namespace JPH
{
  class BodyInterface;
}

/// \brief Flags that define which edges of a breakable slab are fixed/anchored in the world.
/// A fixed edge means that shards adjacent to that edge will remain stationary and not fall under gravity.
struct EZ_JOLTPLUGIN_DLL ezJoltBreakableSlabFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    FixedEdgeTop = EZ_BIT(0),    ///< Shards will stick to this edge
    FixedEdgeRight = EZ_BIT(1),  ///< Shards will stick to this edge
    FixedEdgeBottom = EZ_BIT(2), ///< Shards will stick to this edge
    FixedEdgeLeft = EZ_BIT(3),   ///< Shards will stick to this edge

    Default = FixedEdgeTop | FixedEdgeRight | FixedEdgeBottom | FixedEdgeLeft
  };

  struct Bits
  {
    StorageType FixedEdgeTop : 1;
    StorageType FixedEdgeRight : 1;
    StorageType FixedEdgeBottom : 1;
    StorageType FixedEdgeLeft : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltBreakableSlabFlags);

/// \brief The general shape of the breakable slab.
///
/// Can be extended with further shapes, if desired.
struct EZ_JOLTPLUGIN_DLL ezJoltBreakableShape
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Rectangle,
    Triangle,
    Circle,

    Default = Rectangle
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltBreakableShape);

class EZ_JOLTPLUGIN_DLL ezJoltBreakableSlabComponentManager : public ezComponentManager<class ezJoltBreakableSlabComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltBreakableSlabComponentManager(ezWorld* pWorld);
  ~ezJoltBreakableSlabComponentManager();

  virtual void Initialize() override;

private:
  friend class ezJoltWorldModule;
  friend class ezJoltBreakableSlabComponent;

  void Update(const ezWorldModule::UpdateContext& context);

  ezSet<ezComponentHandle> m_RequireBreakage;
  ezSet<ezComponentHandle> m_RequireBreakUpdate;
};

/// \brief Represents a point in world space, where the breakable slab should be shattered.
struct ezShatterPoint
{
  ezVec3 m_vGlobalPosition;
  ezVec3 m_vImpulse;                                                        ///< With which impulse to push aways new, dynamic shards.
  ezUInt32 m_uiShardIdx = ezInvalidIndex;                                   ///< Which shard to break apart.
  float m_fImpactRadius = 0.05f;                                            ///< The size of the shatter point. Only relevant for some patterns.
  float m_fCellSize = 0.4f;                                                 ///< For the cellular (voronoi) pattern, how large to make cells.
  float m_fMakeDynamicRadius = 0.25f;                                       ///< In what radius around the shatter position to always make new shards dynamic. Unsupported shards will become dynamic regardless.
  ezUInt8 m_uiAllowedBreakPatterns = (ezUInt8)ezBreakablePattern::Cellular; ///< With which pattern to potentially break the shard.
};

/// \brief Most of the shatter calculation (and physics collider generation) is done in this task, to prevent performance drops.
///
/// The result may be ready only with 1-3 frames delay.
class ezShatterTask : public ezTask
{
public:
  ezJoltBreakableSlabComponent* m_pComponent = nullptr;
  ezHybridArray<JPH::Ref<JPH::ConvexShape>, 128> m_Shapes;
  ezMeshResourceHandle m_hShardsMesh;
  ezHybridArray<ezShatterPoint, 8> m_ShatterPoints;
  ezVec3 m_vFinalImpulse;

protected:
  virtual void Execute() override;
};


/// \brief Component that represents a destructible slab that can be broken into shards.
///
/// This component creates a breakable surface that can shatter into smaller pieces when hit.
/// The slab can be rectangular, triangular or circular and can be anchored on any of its edges.
/// When broken, the shards become dynamic physics objects that can collide and fall under gravity.
class EZ_JOLTPLUGIN_DLL ezJoltBreakableSlabComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltBreakableSlabComponent, ezRenderComponent, ezJoltBreakableSlabComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltBreakableSlabComponent

public:
  void SetWidth(float fWidth);         // [ property ]
  float GetWidth() const;              // [ property ]

  void SetHeight(float fHeight);       // [ property ]
  float GetHeight() const;             // [ property ]

  void SetThickness(float fThickness); // [ property ]
  float GetThickness() const;          // [ property ]

  /// \brief Sets the UV scaling factor for texture mapping
  void SetUvScale(ezVec2 vScale); // [ property ]
  ezVec2 GetUvScale() const;      // [ property ]

  /// \brief Sets which edges of the slab are fixed/anchored in the world
  void SetFlags(ezBitflags<ezJoltBreakableSlabFlags> flags); // [ property ]
  ezBitflags<ezJoltBreakableSlabFlags> GetFlags() const      // [ property ]
  {
    return m_Flags;
  }

  /// \brief Sets the basic shape of the breakable slab (rectangle, triangle, circle)
  void SetShape(ezEnum<ezJoltBreakableShape> shape); // [ property ]
  ezEnum<ezJoltBreakableShape> GetShape() const      // [ property ]
  {
    return m_Shape;
  }

  /// \brief Restores the slab to its original unbroken state
  void Restore(); // [ scriptable ]

  /// \brief Shatters the entire slab into pieces of roughly the given size
  /// \param fShardSize The approximate size of generated shards
  /// \param vImpulse The impulse to apply to the shards
  void ShatterAll(float fShardSize, const ezVec3& vImpulse); // [ scriptable ]

  /// \brief Shatters the slab using a cellular (Voronoi) pattern around a point
  /// \param vGlobalPosition The world position where to initiate the break
  /// \param fCellSize The approximate size of the cellular shards
  /// \param vImpulse The impulse to apply to the shards
  /// \param fMakeDynamicRadius Radius around the break point where shards become dynamic
  void ShatterCellular(const ezVec3& vGlobalPosition, float fCellSize, const ezVec3& vImpulse, float fMakeDynamicRadius); // [ scriptable ]

  /// \brief Shatters the slab in a radial pattern around a point
  /// \param vGlobalPosition The world position where to initiate the break
  /// \param fImpactRadius The radius of the impact area
  /// \param vImpulse The impulse to apply to the shards
  /// \param fMakeDynamicRadius Radius around the break point where shards become dynamic
  void ShatterRadial(const ezVec3& vGlobalPosition, float fImpactRadius, const ezVec3& vImpulse, float fMakeDynamicRadius); // [ scriptable ]

private:
  friend class ezShatterTask;

  void PrepareBreakAsync(ezDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes, ezArrayPtr<const ezShatterPoint> points);
  void ApplyBreak(ezArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes, const ezMeshResourceHandle& hMesh, const ezVec3& vImpulse);
  void DebugDraw();
  void Cleanup();
  void ReinitMeshes();
  ezMeshResourceHandle CreateShardsMesh() const;
  void PrepareShardColliders(ezUInt32 uiFirstShard, ezDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes) const;
  ezUInt8 CreateShardColliders(ezUInt32 uiFirstShard, ezArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes);
  void DestroyAllShardColliders();
  void DestroyShardCollider(ezUInt32 uiShardIdx, JPH::BodyInterface& jphBodies, bool bUpdateVis);
  void RetrieveShardTransforms();
  void ApplyImpulse(const ezVec3& vImpulse, ezUInt32 uiFirstShard = 0);
  void UpdateShardColliders();
  void WakeUpBodies();
  bool IsPointOnSlab(const ezVec3& vGlobalPosition) const;
  ezUInt32 FindClosestShard(const ezVec3& vGlobalPosition) const;

  void OnMsgPhysicsAddImpulse(ezMsgPhysicsAddImpulse& ref_msg);           // [ msg handler ]
  void OnMsgPhysicContactMsg(ezMsgPhysicContact& ref_msg);                // [ msg handler ]
  void OnMsgPhysicCharacterContact(ezMsgPhysicCharacterContact& ref_msg); // [ msg handler ]

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void BuildMeshResourceFromGeometry(ezGeometry& Geometry, ezMeshResourceDescriptor& MeshDesc, bool bWithSkinningData) const;
  const ezJoltMaterial* GetPhysicsMaterial();

  float m_fGravityFactor = 1.0f;
  float m_fWidth = 1.0f;
  float m_fHeight = 1.0f;
  float m_fThickness = 0.02f;
  ezVec2 m_vUvScale = ezVec2(1.0f);
  float m_fContactReportForceThreshold = 0.0f;

  ezBreakable2D m_Breakable;

  ezMaterialResourceHandle m_hMaterial;
  ezSurfaceResourceHandle m_hSurface;

  ezInt8 m_iSwitchToSkinningState = -1;
  mutable ezInt8 m_iSkinningStateToUse = -1;
  mutable ezMeshResourceHandle m_hSwitchToMesh;
  mutable ezMeshResourceHandle m_hMeshToRender;
  mutable bool m_bSkinningUpdated[2] = {false, false};
  mutable ezUInt8 m_uiShardsSleeping = 200;

  static ezAtomicInteger32 s_iShardMeshCounter;

  ezUInt8 m_uiCollisionLayerStatic = 0;
  ezUInt8 m_uiCollisionLayerDynamic = 0;

  ezUInt32 m_uiUserDataIndexStatic = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndexDynamic = ezInvalidIndex;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;

  ezDynamicArray<ezUInt32> m_ShardBodyIDs;

  ezSkinningState m_SkinningState[2];
  ezBoundingBoxSphere m_Bounds;

  ezEnum<ezJoltBreakableShape> m_Shape;
  ezBitflags<ezJoltBreakableSlabFlags> m_Flags;

  ezHybridArray<ezShatterPoint, 2> m_ShatterPoints;
  ezSharedPtr<ezShatterTask> m_pShatterTask;
};
