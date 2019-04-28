#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <Foundation/Containers/DynamicArray.h>

struct ezPerInstanceData;

struct ezMsgExtractGeometry;
typedef ezComponentManager<class ezInstancedMeshComponent, ezBlockStorageType::Compact> ezInstancedMeshComponentManager;

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

    ezTransform m_transform;

    ezColor m_color;
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

protected:

  virtual ezUInt32 GetExplicitInstanceDataCount() const override;

  ezUInt32 Instances_GetCount() const;
  ezMeshInstanceData Instances_GetValue(ezUInt32 uiIndex) const;
  void Instances_SetValue(ezUInt32 uiIndex, ezMeshInstanceData value);
  void Instances_Insert(ezUInt32 uiIndex, ezMeshInstanceData value);
  void Instances_Remove(ezUInt32 uiIndex);

  void UpdateRenderInstanceData();

  // Unpacked, reflected instance data for editing and ease of access
  ezDynamicArray<ezMeshInstanceData> m_rawInstancedData;
};
