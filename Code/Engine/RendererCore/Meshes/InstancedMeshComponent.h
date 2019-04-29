#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <Foundation/Containers/DynamicArray.h>

struct ezPerInstanceData;
class ezInstancedMeshComponent;
struct ezMsgExtractGeometry;
class ezStreamWriter;
class ezStreamReader;

class EZ_RENDERERCORE_DLL ezMeshInstanceData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshInstanceData, ezReflectedClass);

  public:

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

class EZ_RENDERERCORE_DLL ezInstancedMeshComponentManager
  : public ezComponentManager<class ezInstancedMeshComponent, ezBlockStorageType::Compact>
{
public:
  typedef ezComponentManager<ezInstancedMeshComponent, ezBlockStorageType::Compact> SUPER;

  ezInstancedMeshComponentManager(ezWorld* pWorld);

  void EnqueueUpdate(ezComponentHandle hComponent);

private:

  mutable ezMutex m_Mutex;
  ezDeque<ezComponentHandle> m_RequireUpdate;

protected:

  void OnRenderBegin(ezUInt64 uiFrameCounter);

  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class EZ_RENDERERCORE_DLL ezInstancedMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezInstancedMeshComponent, ezMeshComponentBase, ezInstancedMeshComponentManager);

public:
  ezInstancedMeshComponent();
  ~ezInstancedMeshComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  /// \brief Extracts the render geometry for export etc.
  void OnExtractGeometry(ezMsgExtractGeometry& msg);

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

protected:

  void EnqueueForUpdate();

  virtual ezUInt32 GetExplicitInstanceDataCount() const override;

  ezUInt32 Instances_GetCount() const;
  ezMeshInstanceData Instances_GetValue(ezUInt32 uiIndex) const;
  void Instances_SetValue(ezUInt32 uiIndex, ezMeshInstanceData value);
  void Instances_Insert(ezUInt32 uiIndex, ezMeshInstanceData value);
  void Instances_Remove(ezUInt32 uiIndex);

  void UpdateRenderInstanceData();

  // Unpacked, reflected instance data for editing and ease of access
  ezDynamicArray<ezMeshInstanceData> m_rawInstancedData;

  mutable bool m_bInUpdateQueue = false;
};
