#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>

typedef ezComponentManagerSimple<class ezAudioListenerComponent, ezComponentUpdateType::WhenSimulating> ezAudioListenerComponentManager;

/// \brief Component used to add an audio listener in the scene, allowing to render sounds.
///
/// The audio listener component can optionally use a position game object and/or an
/// orientation game object. They allow to have more control on how the listener is
/// positioned in the world, to have better results on some cases (ex: First Person and Third Person
/// differs on listener positioning).
///
/// If the position and/or the orientation game object is not set, the component defaults to the owner's
/// position and/or orientation.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioListenerComponent : public ezAudioSystemComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAudioListenerComponent, ezAudioSystemComponent, ezAudioListenerComponentManager);

  // ezComponent

public:
  void OnActivated() override;
  void OnDeactivated() override;
  void SerializeComponent(ezWorldWriter& stream) const override;
  void DeserializeComponent(ezWorldReader& stream) override;

  // ezAudioSystemComponent

private:
  void ezAudioSystemComponentIsAbstract() override {}

  // ezAudioListenerComponent

public:
  ezAudioListenerComponent();
  ~ezAudioListenerComponent() override;

  /// \brief Sets the GUID of the game object which will provide position
  /// data to this listener. If not specified, the owner game object position
  /// is used instead.
  void SetListenerPositionObject(const char* szGuid);

  /// \brief Sets the GUID of the game object which will provide orientation
  /// data to this listener. If not specified, the owner game object orientation
  /// is used instead.
  void SetListenerOrientationObject(const char* szGuid);

  /// \brief Gets the current position of this listener.
  ezVec3 GetListenerPosition();

  /// \brief Gets the current velocity of this listener.
  ezVec3 GetListenerVelocity();

  /// \brief Gets the current orientation of this listener.
  ezQuat GetListenerRotation();

protected:
  void Update();

private:
  const char* _DoNotCall() const;

  ezVec3 m_vListenerPositionOffset;

  ezGameObjectHandle m_hListenerPositionObject;
  ezGameObjectHandle m_hListenerRotationObject;
  ezUInt32 m_uiListenerId;
};
