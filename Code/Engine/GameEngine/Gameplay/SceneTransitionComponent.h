#pragma once

#include <Core/World/EventMessageHandlerComponent.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgTriggerTriggered;

using ezSceneTransitionComponentManager = ezComponentManager<class ezSceneTransitionComponent, ezBlockStorageType::Compact>;

/// \brief What ezSceneTransitionComponent should do when it gets triggered through a ezMsgTriggerTriggered
struct ezSceneLoadMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,          ///< Do nothing, ignore trigger messages.
    LoadAndSwitch, ///< Immediately switch to the target scene, show a loading screen, if necessary.
    Preload,       ///< Start preloading the target scene.
    CancelPreload, ///< Cancel any previously preloaded scene loading.

    Default = LoadAndSwitch
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezSceneLoadMode);

class EZ_GAMEENGINE_DLL ezSceneTransitionComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSceneTransitionComponent, ezComponent, ezSceneTransitionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSceneTransitionComponent

public:
  ezSceneTransitionComponent();
  ~ezSceneTransitionComponent();

  ezHashedString m_sTargetScene;                                                    // [ property ]
  ezHashedString m_sSpawnPoint;                                                     // [ property ]
  bool m_bRelativeSpawnPosition = true;                                             // [ property ]
  ezHashedString m_sPreloadCollectionFile;                                          // [ property ]
  ezEnum<ezSceneLoadMode> m_Mode;                                                   // [ property ]

  void StartTransition(const ezVec3& positionOffset, const ezQuat& rotationOffset); // [ scriptable ]
  void StartPreload();                                                              // [ scriptable ]
  void CancelPreload();                                                             // [ scriptable ]

protected:
  void OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg);
};
