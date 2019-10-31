#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <RendererCore/Meshes/MeshComponent.h>

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcVertexColorRenderData, ezMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  ezGALBufferHandle m_hVertexColorBuffer;
  ezUInt32 m_uiBufferAccessData = 0;
};

//////////////////////////////////////////////////////////////////////////

class ezProcVertexColorComponent;

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorComponentManager
  : public ezComponentManager<ezProcVertexColorComponent, ezBlockStorageType::Compact>
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
  void UpdateComponentVertexColors(ezProcVertexColorComponent* pComponent);
  void OnEndExtraction(ezUInt64 uiFrameCounter);
  void OnBeginRender(ezUInt64 uiFrameCounter);

  void EnqueueUpdate(ezProcVertexColorComponent* pComponent);  
  void RemoveComponent(ezProcVertexColorComponent* pComponent);

  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  void OnAreaInvalidated(const ezProcGenInternal::InvalidatedArea& area);

  ezDynamicArray<ezComponentHandle> m_ComponentsToUpdate;

  ezDynamicArray<ezUniquePtr<ezProcGenInternal::VertexColorTask>> m_UpdateTasks;
  ezTaskGroupID m_UpdateTaskGroupID;
  ezUInt32 m_uiNextTaskIndex = 0;

  ezGALBufferHandle m_hVertexColorBuffer;
  ezDynamicArray<ezUInt32> m_VertexColorData;
  ezUInt32 m_uiCurrentBufferOffset = 0;

  ezGAL::ModifiedRange m_ModifiedDataRange;

  struct DataCopy
  {
    ezArrayPtr<ezUInt32> m_Data;
    ezUInt32 m_uiStart = 0;
  };
  DataCopy m_DataCopy[2];
};

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

  void SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void SetResource(const ezProcGenGraphResourceHandle& hResource);
  const ezProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  const char* GetOutputName(ezUInt32 uiIndex) const;
  void SetOutputName(ezUInt32 uiIndex, const char* szName);

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnTransformChanged(ezMsgTransformChanged& msg);

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;

private:
  ezUInt32 OutputNames_GetCount() const;
  void OutputNames_Insert(ezUInt32 uiIndex, const char* szName);
  void OutputNames_Remove(ezUInt32 uiIndex);

  bool HasValidOutputs() const;

  ezProcGenGraphResourceHandle m_hResource;
  ezHybridArray<ezHashedString, 2> m_OutputNames;

  ezHybridArray<ezSharedPtr<const ezProcGenInternal::VertexColorOutput>, 2> m_Outputs;

  ezGALBufferHandle m_hVertexColorBuffer;
  ezUInt32 m_uiBufferAccessData = 0;
};
