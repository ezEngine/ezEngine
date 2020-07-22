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
  void SetLocalPosition(ezVec3 position);
  ezVec3 GetLocalPosition() const;

  void SetLocalRotation(ezQuat rotation);
  ezQuat GetLocalRotation() const;

  void SetLocalScaling(ezVec3 scaling);
  ezVec3 GetLocalScaling() const;

  ezResult Serialize(ezStreamWriter& writer) const;
  ezResult Deserialize(ezStreamReader& reader);

  ezTransform m_transform;

  ezColor m_color;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshInstanceData);

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezInstancedMeshRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInstancedMeshRenderData, ezMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  ezInstanceData* m_pExplicitInstanceData = nullptr;
  ezUInt32 m_uiExplicitInstanceCount = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezInstancedMeshComponentManager : public ezComponentManager<class ezInstancedMeshComponent, ezBlockStorageType::Compact>
{
public:
  typedef ezComponentManager<ezInstancedMeshComponent, ezBlockStorageType::Compact> SUPER;

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

class EZ_RENDERERCORE_DLL ezInstancedMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezInstancedMeshComponent, ezMeshComponentBase, ezInstancedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

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
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg); // [ msg handler ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezUInt32 Instances_GetCount() const;                                 // [ property ]
  ezMeshInstanceData Instances_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void Instances_SetValue(ezUInt32 uiIndex, ezMeshInstanceData value); // [ property ]
  void Instances_Insert(ezUInt32 uiIndex, ezMeshInstanceData value);   // [ property ]
  void Instances_Remove(ezUInt32 uiIndex);                             // [ property ]

  ezArrayPtr<ezPerInstanceData> GetInstanceData() const;

  // Unpacked, reflected instance data for editing and ease of access
  ezDynamicArray<ezMeshInstanceData> m_rawInstancedData;

  ezInstanceData* m_pExplicitInstanceData = nullptr;

  mutable ezUInt64 m_uiEnqueuedFrame = 0;
};
