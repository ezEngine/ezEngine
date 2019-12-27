#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/GameEngineDLL.h>

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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const override;


  //////////////////////////////////////////////////////////////////////////
  // ezEventMessageHandlerComponent

protected:
  virtual bool HandlesEventMessage(const ezEventMessage& msg) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezVisualScriptComponent

public:
  ezVisualScriptComponent();
  ~ezVisualScriptComponent();

  void SetScriptFile(const char* szFile); // [ property ]
  const char* GetScriptFile() const;      // [ property ]

  void SetScript(const ezVisualScriptResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezVisualScriptResourceHandle& GetScript() const { return m_hResource; }

  const ezRangeView<const char*, ezUInt32> GetParameters() const;   // [ property ]
  void SetParameter(const char* szKey, const ezVariant& value);     // [ property ]
  void RemoveParameter(const char* szKey);                          // [ property ]
  bool GetParameter(const char* szKey, ezVariant& out_value) const; // [ property ]

  static const ezEvent<const ezVisualScriptComponentActivityEvent&>& GetActivityEvents() { return s_ActivityEvents; }

protected:
  void Update();

  static ezEvent<const ezVisualScriptComponentActivityEvent&> s_ActivityEvents;

  struct NumberParam
  {
    EZ_DECLARE_POD_TYPE();
    ezHashedString m_sName;
    double m_Value;
  };

  struct BoolParam
  {
    EZ_DECLARE_POD_TYPE();
    ezHashedString m_sName;
    bool m_Value;
  };

  bool m_bNumberParamsChanged = false;
  bool m_bBoolParamsChanged = false;
  ezHybridArray<NumberParam, 2> m_NumberParams;
  ezHybridArray<BoolParam, 2> m_BoolParams;

  ezVisualScriptResourceHandle m_hResource;
  ezUniquePtr<ezVisualScriptInstance> m_Script;

  bool m_bHadEmptyActivity = true;

  ezUniquePtr<ezVisualScriptInstanceActivity> m_pActivity;
};
