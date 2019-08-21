#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct ezMsgTransformChanged;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezProcVolumeComponent, ezComponent);

public:
  ezProcVolumeComponent();
  ~ezProcVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  float GetValue() const { return m_fValue; }
  void SetValue(float fValue);

  ezEnum<ezProcGenBlendMode> GetBlendMode() const { return m_BlendMode; }
  void SetBlendMode(ezEnum<ezProcGenBlendMode> blendMode);

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnTransformChanged(ezMsgTransformChanged& msg);

private:
  float m_fValue = 1.0f;
  ezEnum<ezProcGenBlendMode> m_BlendMode;
};

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManager<class ezProcVolumeBoxComponent, ezBlockStorageType::Compact> ezProcVolumeBoxComponentManager;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeBoxComponent : public ezProcVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVolumeBoxComponent, ezProcVolumeComponent, ezProcVolumeBoxComponentManager);

public:
  ezProcVolumeBoxComponent();
  ~ezProcVolumeBoxComponent();

private:

};
