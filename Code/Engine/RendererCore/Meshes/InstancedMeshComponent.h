#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

struct ezPerInstanceData;
struct ezRenderWorldRenderEvent;
class ezInstancedMeshComponent;
struct ezMsgExtractGeometry;
class ezStreamWriter;
class ezStreamReader;

struct EZ_RENDERERCORE_DLL ezMeshInstanceData
{
  void SetLocalPosition(ezVec3 vPosition);
  ezVec3 GetLocalPosition() const;

  void SetLocalRotation(ezQuat qRotation);
  ezQuat GetLocalRotation() const;

  void SetLocalScaling(ezVec3 vScaling);
  ezVec3 GetLocalScaling() const;

  ezResult Serialize(ezStreamWriter& ref_writer) const;
  ezResult Deserialize(ezStreamReader& ref_reader);

  ezTransform m_transform;

  ezColor m_color;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshInstanceData);

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezInstancedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInstancedMeshRenderData, ezMeshRenderData);

public:
  virtual bool CanBatch(const ezRenderData& other) const override { return false; }

  ezInstanceData* m_pExplicitInstanceData = nullptr;
  ezUInt32 m_uiExplicitInstanceCount = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezInstancedMeshComponentManager : public ezComponentManager<class ezInstancedMeshComponent, ezBlockStorageType::Compact>
{
public:
  using SUPER = ezComponentManager<ezInstancedMeshComponent, ezBlockStorageType::Compact>;

  ezInstancedMeshComponentManager(ezWorld* pWorld);

  void EnqueueUpdate(const ezInstancedMeshComponent* pComponent) const;

private:
  struct ComponentToUpdate
  {
    ezComponentHandle m_hComponent;
    ezArrayPtr<ezPerInstanceData> m_InstanceData;
  };

  mutable ezMutex m_Mutex;
  mutable ezDeque<ComponentToUpdate> m_RequireUpdate;

protected:
  void OnRenderEvent(const ezRenderWorldRenderEvent& e);

  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

/// \brief Renders multiple instances of the same mesh.
///
/// This is used as an optimization to render many instances of the same (usually small mesh).
/// For example, if you need to render 1000 pieces of grass in a small area,
/// instead of creating 1000 game objects each with a mesh component,
/// it is more efficient to create one game object with an instanced mesh component and give it the locations of the 1000 pieces.
/// Due to the small area, there is no benefit in culling the instances separately.
///
/// However, editing instanced mesh components isn't very convenient, so usually this component would be created and configured
/// in code, rather than by hand in the editor. For example a procedural plant placement system could use this.
class EZ_RENDERERCORE_DLL ezInstancedMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezInstancedMeshComponent, ezMeshComponentBase, ezInstancedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponentBase

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;

  //////////////////////////////////////////////////////////////////////////
  // ezInstancedMeshComponent

public:
  ezInstancedMeshComponent();
  ~ezInstancedMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg); // [ msg handler ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezUInt32 Instances_GetCount() const;                                 // [ property ]
  ezMeshInstanceData Instances_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void Instances_SetValue(ezUInt32 uiIndex, ezMeshInstanceData value); // [ property ]
  void Instances_Insert(ezUInt32 uiIndex, ezMeshInstanceData value);   // [ property ]
  void Instances_Remove(ezUInt32 uiIndex);                             // [ property ]

  ezArrayPtr<ezPerInstanceData> GetInstanceData() const;

  // Unpacked, reflected instance data for editing and ease of access
  ezDynamicArray<ezMeshInstanceData> m_RawInstancedData;

  ezInstanceData* m_pExplicitInstanceData = nullptr;

  mutable ezUInt64 m_uiEnqueuedFrame = ezUInt64(-1);
};
