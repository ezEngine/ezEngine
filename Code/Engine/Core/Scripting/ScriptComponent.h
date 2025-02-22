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

  //////////////////////////////////////////////////////////////////////////
  // ezScriptComponent
public:
  ezScriptComponent();
  ~ezScriptComponent();

  void SetScriptVariable(const ezHashedString& sName, const ezVariant& value);         // [ scriptable ]
  ezVariant GetScriptVariable(const ezHashedString& sName) const;                      // [ scriptable ]

  void SetScriptClass(const ezScriptClassResourceHandle& hScript);                     // [ property ]
  const ezScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; } // [ property ]

  void SetUpdateInterval(ezTime interval);                                             // [ property ]
  ezTime GetUpdateInterval() const;                                                    // [ property ]

  void BroadcastEventMsg(ezEventMessage& ref_msg);

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

  EZ_ALWAYS_INLINE ezScriptInstance* GetScriptInstance() { return m_pInstance.Borrow(); }

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void AddUpdateFunctionToSchedule();
  void RemoveUpdateFunctionToSchedule();

  const ezAbstractFunctionProperty* GetScriptFunction(ezUInt32 uiFunctionIndex);
  void CallScriptFunction(ezUInt32 uiFunctionIndex);

  void ReloadScript();

  ezArrayMap<ezHashedString, ezVariant> m_Parameters;

  ezScriptClassResourceHandle m_hScriptClass;
  ezTime m_UpdateInterval = ezTime::MakeZero();

  ezSharedPtr<ezScriptRTTI> m_pScriptType;
  ezUniquePtr<ezScriptInstance> m_pInstance;

private:
  struct EventSender
  {
    const ezRTTI* m_pMsgType = nullptr;
    ezEventMessageSender<ezEventMessage> m_Sender;
  };

  ezSmallArray<EventSender, 1> m_EventSenders;
};
