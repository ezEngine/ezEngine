#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgUpdateLocalBounds;

using ezBlackboardTemplateResourceHandle = ezTypedResourceHandle<class ezBlackboardTemplateResource>;

class EZ_GAMEENGINE_DLL ezVolumeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezVolumeComponent, ezComponent);

public:
  ezVolumeComponent();
  ~ezVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetTemplateFile(const char* szFile); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]

  void SetTemplate(const ezBlackboardTemplateResourceHandle& hResource);
  ezBlackboardTemplateResourceHandle GetTemplate() const { return m_hTemplateResource; }

  void SetSortOrder(float fOrder);// [ property ]
  float GetSortOrder() const { return m_fSortOrder; }// [ property ]

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  ezBlackboardTemplateResourceHandle m_hTemplateResource;
  float m_fSortOrder = 0.0f;
};

//////////////////////////////////////////////////////////////////////////

using ezVolumeSphereComponentManager = ezComponentManager<class ezVolumeSphereComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezVolumeSphereComponent : public ezVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVolumeSphereComponent, ezVolumeComponent, ezVolumeSphereComponentManager);

public:
  ezVolumeSphereComponent();
  ~ezVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void SetFalloff(float fFalloff);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using ezVolumeBoxComponentManager = ezComponentManager<class ezVolumeBoxComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezVolumeBoxComponent : public ezVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVolumeBoxComponent, ezVolumeComponent, ezVolumeBoxComponentManager);

public:
  ezVolumeBoxComponent();
  ~ezVolumeBoxComponent();

  const ezVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const ezVec3& vExtents);

  const ezVec3& GetFalloff() const { return m_vFalloff; }
  void SetFalloff(const ezVec3& vFalloff);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;

protected:
  ezVec3 m_vExtents = ezVec3(10.0f);
  ezVec3 m_vFalloff = ezVec3(0.5f);
};
