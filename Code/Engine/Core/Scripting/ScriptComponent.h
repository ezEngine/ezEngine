#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Foundation/Types/RangeView.h>

using ezScriptComponentManager = ezComponentManager<class ezScriptComponent, ezBlockStorageType::FreeList>;

class EZ_CORE_DLL ezScriptComponent : public ezEventMessageHandlerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezScriptComponent, ezEventMessageHandlerComponent, ezScriptComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  /*virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const override;

  bool HandleUnhandledMessage(ezMessage& msg, bool bWasPostedMsg);*/

  //////////////////////////////////////////////////////////////////////////
  // ezEventMessageHandlerComponent

protected:
  // virtual bool HandlesEventMessage(const ezEventMessage& msg) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezScriptComponent
public:
  ezScriptComponent();
  ~ezScriptComponent();

  void BroadcastEventMsg(ezEventMessage& msg);

  void SetScriptClass(const ezScriptClassResourceHandle& hScript);
  const ezScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; }

  void SetScriptClassFile(const char* szFile); // [ property ]
  const char* GetScriptClassFile() const;      // [ property ]

  void SetUpdateInterval(ezTime interval); // [ property ]
  ezTime GetUpdateInterval() const;        // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void UpdateScheduling();

  const ezAbstractFunctionProperty* GetScriptFunction(ezUInt32 uiFunctionIndex);
  void CallScriptFunction(ezUInt32 uiFunctionIndex);

  struct EventSender
  {
    const ezRTTI* m_pMsgType = nullptr;
    ezEventMessageSender<ezEventMessage> m_Sender;
  };

  ezHybridArray<EventSender, 2> m_EventSenders;

  ezArrayMap<ezHashedString, ezVariant> m_Parameters;

  ezScriptClassResourceHandle m_hScriptClass;
  ezTime m_UpdateInterval = ezTime::Zero();

  ezSharedPtr<ezScriptRTTI> m_pScriptType;
  ezUniquePtr<ezScriptInstance> m_pInstance;
};
