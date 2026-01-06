#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class ezMeshComponentBase;
class ezProcVertexColorComponent;
using ezCpuMeshResourceHandle = ezTypedResourceHandle<class ezCpuMeshResource>;

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorComponentManager : public ezComponentManager<ezProcVertexColorComponent, ezBlockStorageType::Compact>
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProcVertexColorComponentManager);

public:
  ezProcVertexColorComponentManager(ezWorld* pWorld);
  ~ezProcVertexColorComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezProcVertexColorComponent;

  struct UpdateContext
  {
    ezProcVertexColorComponent* m_pComponent = nullptr;
    ezCpuMeshResourceHandle m_hCpuMesh;
    ezUInt32 m_uiVertexColorOffset = 0;
  };

  void UpdateVertexColors(const ezWorldModule::UpdateContext& context);
  bool UpdateComponentOutputs(ezProcVertexColorComponent& component);
  void UpdateComponentVertexColors(const UpdateContext& context, ezGALDynamicBuffer& buffer);

  void EnqueueUpdate(ezProcVertexColorComponent& component);
  void RemoveComponent(ezProcVertexColorComponent& component);

  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  void OnAreaInvalidated(const ezProcGenInternal::InvalidatedArea& area);

  ezGALDynamicBufferHandle GetVertexColorBuffer();

  ezDynamicArray<ezComponentHandle> m_ComponentsToUpdate;
  ezDynamicArray<UpdateContext> m_UpdateContexts;

  ezDynamicArray<ezSharedPtr<ezProcGenInternal::VertexColorTask>> m_UpdateTasks;
  ezTaskGroupID m_UpdateTaskGroupID;
  ezUInt32 m_uiNextTaskIndex = 0;

  ezUInt32 m_uiCustomDataIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

struct ezProcVertexColorOutputDesc
{
  ezHashedString m_sName;
  ezProcVertexColorMapping m_Mapping;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcVertexColorOutputDesc);

//////////////////////////////////////////////////////////////////////////

struct ezMsgTransformChanged;

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVertexColorComponent, ezComponent, ezProcVertexColorComponentManager);

public:
  ezProcVertexColorComponent();
  ~ezProcVertexColorComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(ezStringView sFile);
  ezStringView GetResourceFile() const;

  void SetResource(const ezProcGenGraphResourceHandle& hResource);
  const ezProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  const ezProcVertexColorOutputDesc& GetOutputDesc(ezUInt32 uiIndex) const;
  void SetOutputDesc(ezUInt32 uiIndex, const ezProcVertexColorOutputDesc& outputDesc);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnMsgTransformChanged(ezMsgTransformChanged& ref_msg);                               // [ msg handler ]
  void OnMsgCustomInstanceDataOffsetChanged(ezMsgCustomInstanceDataOffsetChanged& ref_msg); // [ msg handler ]

private:
  ezUInt32 OutputDescs_GetCount() const;
  void OutputDescs_Insert(ezUInt32 uiIndex, const ezProcVertexColorOutputDesc& outputDesc);
  void OutputDescs_Remove(ezUInt32 uiIndex);

  bool HasValidOutputs() const;

  ezMeshComponentBase* GetMeshComponent();

  ezProcGenGraphResourceHandle m_hResource;
  ezSmallArray<ezProcVertexColorOutputDesc, 1> m_OutputDescs;

  ezSmallArray<ezSharedPtr<const ezProcGenInternal::VertexColorOutput>, 1> m_Outputs;

  ezCustomInstanceDataOffset m_CustomInstanceDataOffset;
};
