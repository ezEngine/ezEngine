#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/GameEngineDLL.h>

class ezVisualScriptComponent;
class ezVisualScriptInstance;
struct ezVisualScriptInstanceActivity;

using ezVisualScriptResourceHandle = ezTypedResourceHandle<class ezVisualScriptResource>;

struct EZ_GAMEENGINE_DLL ezVisualScriptComponentActivityEvent
{
  ezVisualScriptComponent* m_pComponent = nullptr;
  ezVisualScriptInstanceActivity* m_pActivity = nullptr;
};

class EZ_GAMEENGINE_DLL ezVisualScriptComponentManager : public ezComponentManager<ezVisualScriptComponent, ezBlockStorageType::Compact>
{
public:
  ezVisualScriptComponentManager(ezWorld* pWorld);
  ~ezVisualScriptComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezSet<ezComponentHandle> m_ComponentsToUpdate;
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

  virtual void Initialize() override;

  //////////////////////////////////////////////////////////////////////////
  // ezEventMessageHandlerComponent

protected:
  virtual bool HandlesEventMessage(const ezEventMessage& msg) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezVisualScriptComponent

public:
  ezVisualScriptComponent();
  ezVisualScriptComponent(ezVisualScriptComponent&& other);
  ~ezVisualScriptComponent();

  ezVisualScriptComponent& operator=(ezVisualScriptComponent&& other);

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
  void InitScriptInstance();

  static ezEvent<const ezVisualScriptComponentActivityEvent&> s_ActivityEvents;

  struct Param
  {
    ezHashedString m_sName;
    ezVariant m_Value;
  };

  ezHybridArray<Param, 4> m_Params;

  ezVisualScriptResourceHandle m_hResource;
  ezUniquePtr<ezVisualScriptInstance> m_pScriptInstance;

  bool m_bHadEmptyActivity = true;
  bool m_bParamsChanged = false;

  ezUniquePtr<ezVisualScriptInstanceActivity> m_pActivity;
};
