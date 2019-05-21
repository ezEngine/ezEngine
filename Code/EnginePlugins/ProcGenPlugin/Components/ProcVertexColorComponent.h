#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

class ezProcVertexColorComponent;

//////////////////////////////////////////////////////////////////////////

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

  void AddComponent(ezProcVertexColorComponent* pComponent);
  void RemoveComponent(ezProcVertexColorComponent* pComponent);

  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  ezDynamicArray<ezComponentHandle> m_ComponentsToUpdate;
};

//////////////////////////////////////////////////////////////////////////

class EZ_PROCGENPLUGIN_DLL ezProcVertexColorComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVertexColorComponent, ezComponent, ezProcVertexColorComponentManager);

public:
  ezProcVertexColorComponent();
  ~ezProcVertexColorComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void SetResource(const ezProcGenGraphResourceHandle& hResource);
  const ezProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  ezProcGenGraphResourceHandle m_hResource;

};
