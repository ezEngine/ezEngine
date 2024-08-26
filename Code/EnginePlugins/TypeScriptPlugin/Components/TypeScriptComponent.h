#pragma once

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Core/World/Component.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezTypeScriptBinding;

struct EZ_TYPESCRIPTPLUGIN_DLL ezMsgTypeScriptMsgProxy : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgTypeScriptMsgProxy, ezMessage);

  ezUInt32 m_uiTypeNameHash = 0;
  ezUInt32 m_uiStashIndex = 0;
};

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptComponentManager : public ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList>
{
  using SUPER = ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList>;

public:
  ezTypeScriptComponentManager(ezWorld* pWorld);
  ~ezTypeScriptComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  ezTypeScriptBinding& GetTsBinding() const { return m_TsBinding; }

private:
  void Update(const ezWorldModule::UpdateContext& context);

  mutable ezTypeScriptBinding m_TsBinding;
};

//////////////////////////////////////////////////////////////////////////

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptComponent : public ezEventMessageHandlerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTypeScriptComponent, ezEventMessageHandlerComponent, ezTypeScriptComponentManager);

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

  virtual bool HandlesMessage(const ezMessage& msg) const override;
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const override;

  bool HandleUnhandledMessage(ezMessage& msg, bool bWasPostedMsg);

  //////////////////////////////////////////////////////////////////////////
  // ezTypeScriptComponent

public:
  ezTypeScriptComponent();
  ~ezTypeScriptComponent();

  void BroadcastEventMsg(ezEventMessage& ref_msg);

  void SetUpdateInterval(ezTime interval) { m_UpdateInterval = interval; }

  void SetTypeScriptComponentGuid(const ezUuid& resource);
  const ezUuid& GetTypeScriptComponentGuid() const;

private:
  struct EventSender
  {
    const ezRTTI* m_pMsgType = nullptr;
    ezEventMessageSender<ezEventMessage> m_Sender;
  };

  ezHybridArray<EventSender, 2> m_EventSenders;

  bool CallTsFunc(const char* szFuncName);
  void Update(ezTypeScriptBinding& script);
  void SetExposedVariables();

  ezTypeScriptBinding::TsComponentTypeInfo m_ComponentTypeInfo;

  void SetTypeScriptComponentFile(const char* szFile);        // [ property ]
  const char* GetTypeScriptComponentFile() const;             // [ property ]

  void OnMsgTypeScriptMsgProxy(ezMsgTypeScriptMsgProxy& msg); // [ message handler ]

  enum UserFlag
  {
    InitializedTS = 0,
    OnActivatedTS = 1,
    NoTsTick = 2,
    SimStartedTS = 3,
    ScriptFailure = 4,
  };

private:
  ezUuid m_TypeScriptComponentGuid;
  ezTime m_LastUpdate;
  ezTime m_UpdateInterval = ezTime::MakeZero();

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters

public:
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
};
