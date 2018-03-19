#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <Core/World/World.h>
#include <Core/Graphics/Camera.h>

class ezView;
struct ezResourceEvent;

class EZ_RENDERERCORE_DLL ezCameraComponentManager : public ezComponentManager<class ezCameraComponent, ezBlockStorageType::Compact>
{
public:
  ezCameraComponentManager(ezWorld* pWorld);
  ~ezCameraComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void Update(const ezWorldModule::UpdateContext& context);

  const ezCameraComponent* GetCameraByUsageHint(ezCameraUsageHint::Enum usageHint) const;
  ezCameraComponent* GetCameraByUsageHint(ezCameraUsageHint::Enum usageHint);

private:
  friend class ezCameraComponent;

  void AddRenderTargetCamera(ezCameraComponent* pComponent);
  void RemoveRenderTargetCamera(ezCameraComponent* pComponent);

  void OnViewCreated(ezView* pView);

  ezDynamicArray<ezComponentHandle> m_modifiedCameras;
  ezDynamicArray<ezComponentHandle> m_RenderTargetCameras;
};


class EZ_RENDERERCORE_DLL ezCameraComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraComponent, ezComponent, ezCameraComponentManager);

public:
  ezCameraComponent();
  ~ezCameraComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface
  // 

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraComponent interface
  // 

public:
  void UpdateRenderTargetCamera();

  ezEnum<ezCameraUsageHint> GetUsageHint() const { return m_UsageHint; }
  void SetUsageHint(ezEnum<ezCameraUsageHint> val);

  void SetRenderTargetFile(const char* szFile);
  const char* GetRenderTargetFile() const;

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
  void ResourceChangeEventHandler(const ezResourceEvent& e);

  ezEnum<ezCameraUsageHint> m_UsageHint;
  ezEnum<ezCameraMode> m_Mode;
  ezTexture2DResourceHandle m_hRenderTarget;
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
  void MarkAsModified(ezCameraComponentManager* pCameraManager);

  bool m_bIsModified;
  bool m_bShowStats;
  bool m_bRenderTargetInitialized = false;

  void ActivateRenderToTexture();
  void DeactivateRenderToTexture();

  ezViewHandle m_hRenderTargetView;
  ezCamera m_RenderTargetCamera;
};
