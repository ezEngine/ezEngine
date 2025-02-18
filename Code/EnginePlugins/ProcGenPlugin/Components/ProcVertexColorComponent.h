#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcVertexColorRenderData, ezMeshRenderData);

public:
  ezGALDynamicBufferHandle m_hVertexColorBuffer;
  ezUInt32 m_uiBufferAccessData = 0;
};

//////////////////////////////////////////////////////////////////////////

struct ezRenderWorldExtractionEvent;
class ezProcVertexColorComponent;
class ezCpuMeshResource;

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

  void UpdateVertexColors(const ezWorldModule::UpdateContext& context);
  bool UpdateComponentOutputs(ezProcVertexColorComponent* pComponent);
  void UpdateComponentVertexColors(ezProcVertexColorComponent* pComponent, ezGALDynamicBuffer* pBuffer);
  void OnExtractionEvent(const ezRenderWorldExtractionEvent& e);

  void EnqueueUpdate(ezProcVertexColorComponent* pComponent);
  void RemoveComponent(ezProcVertexColorComponent* pComponent);

  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  void OnAreaInvalidated(const ezProcGenInternal::InvalidatedArea& area);

  ezDynamicArray<ezComponentHandle> m_ComponentsToUpdate;

  ezDynamicArray<ezSharedPtr<ezProcGenInternal::VertexColorTask>> m_UpdateTasks;
  ezTaskGroupID m_UpdateTaskGroupID;
  ezUInt32 m_uiNextTaskIndex = 0;

  ezGALDynamicBufferHandle m_hVertexColorBuffer;
};

//////////////////////////////////////////////////////////////////////////

struct ezProcVertexColorOutputDesc
{
  ezHashedString m_sName;
  ezProcVertexColorMapping m_Mapping;

  void SetName(const char* szName);
  const char* GetName() const { return m_sName; }

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcVertexColorOutputDesc);

//////////////////////////////////////////////////////////////////////////

struct ezMsgTransformChanged;

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorComponent : public ezMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVertexColorComponent, ezMeshComponent, ezProcVertexColorComponentManager);

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

  void OnTransformChanged(ezMsgTransformChanged& ref_msg);

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;

private:
  ezUInt32 OutputDescs_GetCount() const;
  void OutputDescs_Insert(ezUInt32 uiIndex, const ezProcVertexColorOutputDesc& outputDesc);
  void OutputDescs_Remove(ezUInt32 uiIndex);

  bool HasValidOutputs() const;

  const ezCpuMeshResource* GetCpuMeshResource() const;

  enum
  {
    BUFFER_ACCESS_OFFSET_BITS = 28,
    BUFFER_ACCESS_OFFSET_MASK = (1 << BUFFER_ACCESS_OFFSET_BITS) - 1,
  };

  void SetBufferOffset(ezUInt32 uiOffset);
  ezUInt32 GetBufferOffset() const { return m_uiBufferAccessData & BUFFER_ACCESS_OFFSET_MASK; }

  ezProcGenGraphResourceHandle m_hResource;
  ezHybridArray<ezProcVertexColorOutputDesc, 2> m_OutputDescs;

  ezHybridArray<ezSharedPtr<const ezProcGenInternal::VertexColorOutput>, 2> m_Outputs;

  ezGALDynamicBufferHandle m_hVertexColorBuffer;
  ezUInt32 m_uiBufferAccessData = 0;
};
