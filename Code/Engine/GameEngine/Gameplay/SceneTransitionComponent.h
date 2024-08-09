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

/// \brief Provides functionality to transition from one scene to another (level loading).
///
/// The component references a target level and spawn point (optional).
/// When triggered, either manually or through a trigger message (e.g. from a physics trigger
/// attached to the same object), the component instructs the active game state to (pre-) load a scene.
///
/// It may also automatically forward the relative position of the player from this object into the
/// target scene, such that the level transition appears more seamless.
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

  /// GUID or path to the scene that shall be loaded.
  ezHashedString m_sTargetScene; // [ property ]

  /// Optional name of the spawn point (see ezPlayerStartPointComponent).
  /// If no spawn point with this name exists, the first one in the scene is used.
  ezHashedString m_sSpawnPoint; // [ property ]

  /// If true, the relative player position in this scene is forwarded to the target scene spawn point.
  /// Thus if the two levels looks the same at the transition point, the transition appears more seamless.
  bool m_bRelativeSpawnPosition = true; // [ property ]

  /// Optional collection file to use for preloading.
  /// Necessary for proper loading progress calculation.
  ezHashedString m_sPreloadCollectionFile; // [ property ]

  /// If not set to 'None' the component reacts to trigger messages with the desired operation.
  /// You can attach a trigger component to the same object (or child) to automatically
  /// switch levels. If this is set to 'None', though, operations have to be triggered
  /// manually through script code or by sending ezMsgTriggerTriggered directly.
  ezEnum<ezSceneLoadMode> m_Mode; // [ property ]

  /// \brief Makes the game immediately switch to the target level.
  ///
  /// If necessary, the loading screen is shown first.
  /// If offsets are given, the player spawns relative to the target spawn point.
  /// This
  void StartTransition(const ezVec3& vPositionOffset = ezVec3::MakeZero(), const ezQuat& qRotationOffset = ezQuat::MakeIdentity()); // [ scriptable ]

  /// \brief Same as StartTransition() but computes the relative offset to this object from the given global transform.
  void StartTransitionWithOffsetTo(const ezVec3& vGlobalPosition, const ezQuat& qGlobalRotation); // [ scriptable ]

  /// \brief Starts preloading the target scene.
  ///
  /// Does not switch to it automatically. Call StartTransition() at any time to do so.
  /// If the preload is finished by then, the transition will be as seamless as possible,
  /// if not, a loading screen may show up.
  void StartPreload(); // [ scriptable ]

  /// \brief Cancels any pending preload and frees up memory that memory.
  ///
  /// Should be used, if the player can enter a preload area and then leave it again, without transitioning the level.
  void CancelPreload(); // [ scriptable ]

protected:
  void OnMsgTriggerTriggered(ezMsgTriggerTriggered& ref_msg);
};
