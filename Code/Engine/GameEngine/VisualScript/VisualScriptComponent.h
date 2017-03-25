#pragma once

#include <Core/World/World.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezVisualScriptInstance;

typedef ezTypedResourceHandle<class ezVisualScriptResource> ezVisualScriptResourceHandle;

typedef ezComponentManagerSimple<class ezVisualScriptComponent, ezComponentUpdateType::WhenSimulating> ezVisualScriptComponentManager;

class ezVisualScriptComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVisualScriptComponent, ezComponent, ezVisualScriptComponentManager);

public:
  ezVisualScriptComponent();
  ~ezVisualScriptComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void SetScriptFile(const char* szFile);
  const char* GetScriptFile() const;

  void SetScript(const ezVisualScriptResourceHandle& hResource);
  EZ_FORCE_INLINE const ezVisualScriptResourceHandle& GetScript() const { return m_hResource; }

  void Update();

protected:
  virtual bool OnUnhandledMessage(ezMessage& msg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg) const override;

  ezVisualScriptResourceHandle m_hResource;
  ezUniquePtr<ezVisualScriptInstance> m_Script;
};


