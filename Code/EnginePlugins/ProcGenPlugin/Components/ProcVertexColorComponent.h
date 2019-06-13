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
  int m_iBufferOffset = -1;
};

//////////////////////////////////////////////////////////////////////////

class ezProcVertexColorComponent;

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorComponentManager
  : public ezComponentManager<ezProcVertexColorComponent, ezBlockStorageType::Compact>
{
public:
  ezProcVertexColorComponentManager(ezWorld* pWorld);
  ~ezProcVertexColorComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezProcVertexColorComponent;

  void UpdateVertexColors(const ezWorldModule::UpdateContext& context);
  void OnEndExtraction(ezUInt64 uiFrameCounter);
  void OnBeginRender(ezUInt64 uiFrameCounter);

  void AddComponent(ezProcVertexColorComponent* pComponent);
  void RemoveComponent(ezProcVertexColorComponent* pComponent);

  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  ezDynamicArray<ezComponentHandle> m_ComponentsToUpdate;

  ezGALBufferHandle m_hVertexColorBuffer;
  ezDynamicArray<ezUInt32> m_VertexColorData;
  ezUInt32 m_uiCurrentBufferOffset = 0;
};

//////////////////////////////////////////////////////////////////////////

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

  void SetOutputName(const char* szOutputName);
  const char* GetOutputName() const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;

private:
  ezProcGenGraphResourceHandle m_hResource;
  ezHashedString m_sOutputName;

  ezSharedPtr<const ezProcGenInternal::VertexColorOutput> m_pOutput;

  ezGALBufferHandle m_hVertexColorBuffer;
  int m_iBufferOffset = INT_MIN;
};
