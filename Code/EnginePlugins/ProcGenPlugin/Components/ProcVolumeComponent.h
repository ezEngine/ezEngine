#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct ezMsgTransformChanged;
struct ezMsgExtractVolumes;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezProcVolumeComponent, ezComponent);

public:
  ezProcVolumeComponent();
  ~ezProcVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetValue(float fValue);
  float GetValue() const { return m_fValue; }

  void SetSortOrder(float fOrder);
  float GetSortOrder() const { return m_fSortOrder; }

  void SetBlendMode(ezEnum<ezProcGenBlendMode> blendMode);
  ezEnum<ezProcGenBlendMode> GetBlendMode() const { return m_BlendMode; }

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnTransformChanged(ezMsgTransformChanged& msg);

  static const ezEvent<const ezProcGenInternal::InvalidatedArea&>& GetAreaInvalidatedEvent() { return s_AreaInvalidatedEvent; }

protected:
  float m_fValue = 1.0f;
  float m_fSortOrder = 0.0f;
  ezEnum<ezProcGenBlendMode> m_BlendMode;

  void InvalidateArea();
  void InvalidateArea(const ezBoundingBox& area);

  static ezEvent<const ezProcGenInternal::InvalidatedArea&> s_AreaInvalidatedEvent;
};

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManager<class ezProcVolumeSphereComponent, ezBlockStorageType::Compact> ezProcVolumeSphereComponentManager;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeSphereComponent : public ezProcVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVolumeSphereComponent, ezProcVolumeComponent, ezProcVolumeSphereComponentManager);

public:
  ezProcVolumeSphereComponent();
  ~ezProcVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFadeOutStart() const { return m_fFadeOutStart; }
  void SetFadeOutStart(float fFadeOutStart);

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  void OnExtractVolumes(ezMsgExtractVolumes& msg) const;

private:
  float m_fRadius = 10.0f;
  float m_fFadeOutStart = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManager<class ezProcVolumeBoxComponent, ezBlockStorageType::Compact> ezProcVolumeBoxComponentManager;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeBoxComponent : public ezProcVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVolumeBoxComponent, ezProcVolumeComponent, ezProcVolumeBoxComponentManager);

public:
  ezProcVolumeBoxComponent();
  ~ezProcVolumeBoxComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
};
