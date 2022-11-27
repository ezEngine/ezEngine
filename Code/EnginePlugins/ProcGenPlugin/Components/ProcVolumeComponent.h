#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct ezMsgTransformChanged;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractVolumes;

using ezImageDataResourceHandle = ezTypedResourceHandle<class ezImageDataResource>;

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

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnTransformChanged(ezMsgTransformChanged& ref_msg);

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

using ezProcVolumeSphereComponentManager = ezComponentManager<class ezProcVolumeSphereComponent, ezBlockStorageType::Compact>;

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

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(ezMsgExtractVolumes& ref_msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFadeOutStart = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using ezProcVolumeBoxComponentManager = ezComponentManager<class ezProcVolumeBoxComponent, ezBlockStorageType::Compact>;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeBoxComponent : public ezProcVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVolumeBoxComponent, ezProcVolumeComponent, ezProcVolumeBoxComponentManager);

public:
  ezProcVolumeBoxComponent();
  ~ezProcVolumeBoxComponent();

  const ezVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const ezVec3& vExtents);

  const ezVec3& GetFadeOutStart() const { return m_vFadeOutStart; }
  void SetFadeOutStart(const ezVec3& vFadeOutStart);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(ezMsgExtractVolumes& ref_msg) const;

protected:
  ezVec3 m_vExtents = ezVec3(10.0f);
  ezVec3 m_vFadeOutStart = ezVec3(0.5f);
};

//////////////////////////////////////////////////////////////////////////

using ezProcVolumeImageComponentManager = ezComponentManager<class ezProcVolumeImageComponent, ezBlockStorageType::Compact>;

class EZ_PROCGENPLUGIN_DLL ezProcVolumeImageComponent : public ezProcVolumeBoxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcVolumeImageComponent, ezProcVolumeBoxComponent, ezProcVolumeImageComponentManager);

public:
  ezProcVolumeImageComponent();
  ~ezProcVolumeImageComponent();

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnExtractVolumes(ezMsgExtractVolumes& ref_msg) const;

  void SetImageFile(const char* szFile); // [ property ]
  const char* GetImageFile() const;      // [ property ]

  void SetImage(const ezImageDataResourceHandle& hResource);
  ezImageDataResourceHandle GetImage() const { return m_hImage; }

protected:
  ezImageDataResourceHandle m_hImage;
};
