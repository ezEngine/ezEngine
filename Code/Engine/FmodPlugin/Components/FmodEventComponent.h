#pragma once

#include <FmodPlugin/Components/FmodComponent.h>

typedef ezComponentManagerSimple<class ezFmodEventComponent, true> ezFmodEventComponentManager;

class EZ_FMODPLUGIN_DLL ezFmodEventComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodEventComponent, ezFmodComponent, ezFmodEventComponentManager);
  virtual void ezFmodComponentIsAbstract() override {}

public:
  ezFmodEventComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  void SetPaused(bool b);
  bool GetPaused() const { return m_bPaused; }

  void SetPitch(float f);
  float GetPitch() const { return m_fPitch; }

  void SetVolume(float f);
  float GetVolume() const { return m_fVolume; }

  ezString m_sEvent;

  /// \todo Event Parameters (expose, modify)

protected:
  bool m_bPaused;
  float m_fPitch;
  float m_fVolume;
  

  // ************************************* FUNCTIONS *****************************

public:
  virtual ezComponent::Initialization Initialize() override;
  virtual void Deinitialize() override;

  void StartOneShot();

  void StopSound();
  void StopSoundImmediate();

protected:

  void Update();
  void SetParameters3d(FMOD::Studio::EventInstance* pEventInstance);

  FMOD::Studio::EventDescription* m_pEventDesc;
  FMOD::Studio::EventInstance* m_pEventInstance;
};


