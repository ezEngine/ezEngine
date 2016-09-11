#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Camera/Declarations.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <CoreUtils/Graphics/Camera.h>

class ezView;

class EZ_RENDERERCORE_DLL ezCameraComponentManager : public ezComponentManager<class ezCameraComponent, true>
{
public:
  ezCameraComponentManager(ezWorld* pWorld);
  ~ezCameraComponentManager();

  virtual void Initialize() override;

  void Update(ezUInt32 uiStartIndex, ezUInt32 uiCount);

  const ezCameraComponent* GetCameraByUsageHint(ezCameraComponentUsageHint::Enum usageHint) const;

private:
  friend class ezCameraComponent;

  ezDynamicArray<ezComponentHandle> m_modifiedCameras;
};


class EZ_RENDERERCORE_DLL ezCameraComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraComponent, ezComponent, ezCameraComponentManager);

public:
  ezCameraComponent();
  ~ezCameraComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  ezEnum<ezCameraComponentUsageHint> GetUsageHint() const { return m_UsageHint; }
  void SetUsageHint(ezEnum<ezCameraComponentUsageHint> val);

  ezEnum<ezCameraMode> GetCameraMode() const { return m_Mode; }
  void SetCameraMode(ezEnum<ezCameraMode> val);

  float GetNearPlane() const { return m_fNearPlane; }
  void SetNearPlane(float val);

  float GetFarPlane() const { return m_fFarPlane; }
  void SetFarPlane(float val);

  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; }
  void SetFieldOfView(float val);

  float GetOrthoDimension() const { return m_fOrthoDimension; }
  void SetOrthoDimension(float val);

  ezRenderPipelineResourceHandle GetRenderPipeline() const { return m_hRenderPipeline; }
  void SetRenderPipeline(ezRenderPipelineResourceHandle hRenderPipeline);

  const char* GetRenderPipelineFile() const;
  void SetRenderPipelineFile(const char* szFile);

  float GetAperture() const { return m_fAperture; }
  void SetAperture(float fAperture);

  float GetShutterTime() const { return m_fShutterTime; }
  void SetShutterTime(float fShutterTime);

  float GetISO() const { return m_fISO; }
  void SetISO(float fISO);

  float GetExposureCompensation() const { return m_fExposureCompensation; }
  void SetExposureCompensation(float fEC);

  float GetEV100() const;
  float GetExposure() const;

  void ApplySettingsToView(ezView* pView) const;

private:
  ezEnum<ezCameraComponentUsageHint> m_UsageHint;
  ezEnum<ezCameraMode> m_Mode;
  float m_fNearPlane;
  float m_fFarPlane;
  float m_fPerspectiveFieldOfView;
  float m_fOrthoDimension;
  ezRenderPipelineResourceHandle m_hRenderPipeline;

  ezTagSet m_IncludeTags;
  ezTagSet m_ExcludeTags;

  float m_fAperture;
  float m_fShutterTime;
  float m_fISO;
  float m_fExposureCompensation;

  void MarkAsModified();
  bool m_bIsModified;
};
