#pragma once

#include <FmodPlugin/Components/FmodComponent.h>
#include <Core/ResourceManager/Resource.h>

typedef ezComponentManagerSimple<class ezFmodEventComponent, true> ezFmodEventComponentManager;
typedef ezTypedResourceHandle<class ezFmodSoundEventResource> ezFmodSoundEventResourceHandle;

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

  void SetSoundEventFile(const char* szFile);
  const char* GetSoundEventFile() const;

  void SetSoundEvent(const ezFmodSoundEventResourceHandle& hSoundEvent);
  const ezFmodSoundEventResourceHandle& GetSoundEvent() const { return m_hSoundEvent; }

  ezOnComponentFinishedAction::Enum m_OnFinishedAction;

  /// \todo Event Parameters (expose, modify)

protected:
  bool m_bPaused;
  float m_fPitch;
  float m_fVolume;
  ezFmodSoundEventResourceHandle m_hSoundEvent;
  

  // ************************************* FUNCTIONS *****************************

public:
  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void Restart();
  void StartOneShot();

  void StopSound();
  void StopSoundImmediate();

protected:

  void Update();
  void SetParameters3d(FMOD::Studio::EventInstance* pEventInstance);

  FMOD::Studio::EventDescription* m_pEventDesc;
  FMOD::Studio::EventInstance* m_pEventInstance;
};


