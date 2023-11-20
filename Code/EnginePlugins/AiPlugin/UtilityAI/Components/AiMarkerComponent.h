#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <GameEngine/Gameplay/MarkerComponent.h>

using ezAiMarkerComponentManager = ezComponentManager<class ezAiMarkerComponent, ezBlockStorageType::Compact>;

/// A marker that carries information for the AI, e.g. the noise of a shot, a footstep or another hint that NPCs can sense.
///
/// Sensors detect it like any other marker. The radius defines from how far away it can be sensed (see ezSensorComponent).
/// The game code places these markers and decides how long they exist, e.g. by enabling a marker on the shooter for a moment,
/// or by spawning a prefab with a marker (plus an ezTimedDeathComponent) where something happened.
/// See ezAiPerceptionGenHint for how the AI turns them into perceptions.
class EZ_AIPLUGIN_DLL ezAiMarkerComponent : public ezMarkerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiMarkerComponent, ezMarkerComponent, ezAiMarkerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAiMarkerComponent

public:
  ezAiMarkerComponent();
  ~ezAiMarkerComponent();

  /// When an NPC senses several hints, the ones with a lower priority are ignored, e.g. footsteps once a shot is heard.
  float m_fPriority = 0.5f; // [ property ]

  /// The object that caused the hint, e.g. the shooter.
  ///
  /// NPCs ignore hints whose source they detect directly (the source itself or any object below it, e.g. the shooter's own marker).
  /// Not serialized, has to be set at runtime, e.g. by the script that spawns the marker.
  void SetSourceObject(ezGameObjectHandle hSource); // [ scriptable ]

  /// Returns the object set with SetSourceObject(). If none was set, or it doesn't exist anymore, the parent of the owner
  /// (the object the marker is attached to, e.g. the character), or the owner itself if it has no parent.
  ezGameObjectHandle GetSourceObject() const; // [ scriptable ]

private:
  ezGameObjectHandle m_hSource;
};
