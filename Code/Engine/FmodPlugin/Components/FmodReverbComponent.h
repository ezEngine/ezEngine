#pragma once

#include <FmodPlugin/Components/FmodComponent.h>

typedef ezComponentManagerSimple<class ezFmodReverbComponent, ezComponentUpdateType::WhenSimulating> ezFmodReverbComponentManager;

struct ezFmodReverbPresets
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Off,
    Generic,
    PaddedCell,
    Room,
    Bathroom,
    Livingroom,
    Stoneroom,
    Auditorium,
    Concerthall,
    Cave,
    Arena,
    Hangar,
    CarpettedHallway,
    Hallway,
    StoneCorridor,
    Alley,
    Forest,
    City,
    Mountains,
    Quarry,
    Plain,
    Parkinlot,
    Sewerpipe,
    Underwater,
    //Custom, /// \todo Do we want to expose manually setting these ? Probably should be a preset editor somewhere instead

    Default = Generic,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FMODPLUGIN_DLL, ezFmodReverbPresets);

class EZ_FMODPLUGIN_DLL ezFmodReverbComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodReverbComponent, ezFmodComponent, ezFmodReverbComponentManager);
  virtual void ezFmodComponentIsAbstract() override {}

public:
  ezFmodReverbComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  float GetMinDistance() const { return m_fMinDistance; }
  void SetMinDistance(float f);

  float GetMaxDistance() const { return m_fMaxDistance; }
  void SetMaxDistance(float f);

protected:

  float m_fMinDistance;
  float m_fMaxDistance;
  ezEnum<ezFmodReverbPresets> m_ReverbPreset;

public:
  virtual void OnSimulationStarted() override;
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

protected:
  void Update();
  void SetParameters3d();

  FMOD::Reverb3D* m_pReverb;
};


