#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezVisualScriptInstance;
struct ezVisualScriptInstanceActivity;

typedef ezTypedResourceHandle<class ezVisualScriptResource> ezVisualScriptResourceHandle;

typedef ezComponentManagerSimple<class ezVisualScriptComponent, ezComponentUpdateType::WhenSimulating> ezVisualScriptComponentManager;

struct EZ_GAMEENGINE_DLL ezVisualScriptComponentActivityEvent
{
  ezVisualScriptComponent* m_pComponent = nullptr;
  ezVisualScriptInstanceActivity* m_pActivity = nullptr;
};

class EZ_GAMEENGINE_DLL ezVisualScriptComponent : public ezEventMessageHandlerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVisualScriptComponent, ezEventMessageHandlerComponent, ezVisualScriptComponentManager);

public:
  ezVisualScriptComponent();
  ~ezVisualScriptComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void SetScriptFile(const char* szFile);
  const char* GetScriptFile() const;

  void SetScript(const ezVisualScriptResourceHandle& hResource);
  EZ_FORCE_INLINE const ezVisualScriptResourceHandle& GetScript() const { return m_hResource; }

  void SetIsGlobalEventHandler(bool enable);
  bool GetIsGlobalEventHandler() const { return m_bGlobalEventHandler; }

  virtual bool HandlesEventMessage(const ezEventMessage& msg) const override;

  void Update();

  static ezEvent<const ezVisualScriptComponentActivityEvent&> s_ActivityEvents;

protected:
  virtual bool OnUnhandledMessage(ezMessage& msg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg) const override;

  ezVisualScriptResourceHandle m_hResource;
  ezUniquePtr<ezVisualScriptInstance> m_Script;

  bool m_bGlobalEventHandler = false;
  bool m_bHadEmptyActivity = true;

  ezUniquePtr<ezVisualScriptInstanceActivity> m_pActivity;
};


