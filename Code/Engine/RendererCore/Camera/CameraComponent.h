#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Camera/Declarations.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <CoreUtils/Graphics/Camera.h>


typedef ezComponentManager<class ezCameraComponent> ezCameraComponentManager;

class EZ_RENDERERCORE_DLL ezCameraComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraComponent, ezComponent, ezCameraComponentManager);

public:
  ezCameraComponent();

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

  /// \brief Returns a counter that is increased every time the camera settings are modified.
  ezUInt32 GetSettingsModificationCounter() const { return m_uiSettingsModificationCounter; }

private:
  ezEnum<ezCameraComponentUsageHint> m_UsageHint;
  ezEnum<ezCameraMode> m_Mode;
  float m_fNearPlane;
  float m_fFarPlane;
  float m_fPerspectiveFieldOfView;
  float m_fOrthoDimension;
  ezRenderPipelineResourceHandle m_hRenderPipeline;

private:
  ezUInt32 m_uiSettingsModificationCounter;

};
