#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

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

  void ReinitializeAllRenderTargetCameras();

  const ezCameraComponent* GetCameraByUsageHint(ezCameraUsageHint::Enum usageHint) const;
  ezCameraComponent* GetCameraByUsageHint(ezCameraUsageHint::Enum usageHint);

private:
  friend class ezCameraComponent;

  void AddRenderTargetCamera(ezCameraComponent* pComponent);
  void RemoveRenderTargetCamera(ezCameraComponent* pComponent);

  void OnViewCreated(ezView* pView);
  void OnCameraConfigsChanged(void* dummy);

  ezDynamicArray<ezComponentHandle> m_ModifiedCameras;
  ezDynamicArray<ezComponentHandle> m_RenderTargetCameras;
};


class EZ_RENDERERCORE_DLL ezCameraComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraComponent, ezComponent, ezCameraComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraComponent

public:
  ezCameraComponent();
  ~ezCameraComponent();

  ezEnum<ezCameraUsageHint> GetUsageHint() const { return m_UsageHint; } // [ property ]
  void SetUsageHint(ezEnum<ezCameraUsageHint> val);                      // [ property ]

  void SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;      // [ property ]

  void SetRenderTargetRectOffset(ezVec2 value);                                 // [ property ]
  ezVec2 GetRenderTargetRectOffset() const { return m_vRenderTargetRectOffset; } // [ property ]

  void SetRenderTargetRectSize(ezVec2 value);                               // [ property ]
  ezVec2 GetRenderTargetRectSize() const { return m_vRenderTargetRectSize; } // [ property ]

  ezEnum<ezCameraMode> GetCameraMode() const { return m_Mode; } // [ property ]
  void SetCameraMode(ezEnum<ezCameraMode> val);                 // [ property ]

  float GetNearPlane() const { return m_fNearPlane; } // [ property ]
  void SetNearPlane(float fVal);                      // [ property ]

  float GetFarPlane() const { return m_fFarPlane; } // [ property ]
  void SetFarPlane(float fVal);                     // [ property ]

  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; } // [ property ]
  void SetFieldOfView(float fVal);                                   // [ property ]

  float GetOrthoDimension() const { return m_fOrthoDimension; } // [ property ]
  void SetOrthoDimension(float fVal);                           // [ property ]

  ezRenderPipelineResourceHandle GetRenderPipeline() const;
  ezViewHandle GetRenderTargetView() const;

  const char* GetRenderPipelineEnum() const;      // [ property ]
  void SetRenderPipelineEnum(const char* szFile); // [ property ]

  float GetAperture() const { return m_fAperture; } // [ property ]
  void SetAperture(float fAperture);                // [ property ]

  ezTime GetShutterTime() const { return m_ShutterTime; } // [ property ]
  void SetShutterTime(ezTime shutterTime);                // [ property ]

  float GetISO() const { return m_fISO; } // [ property ]
  void SetISO(float fISO);                // [ property ]

  float GetExposureCompensation() const { return m_fExposureCompensation; } // [ property ]
  void SetExposureCompensation(float fEC);                                  // [ property ]

  float GetEV100() const;    // [ property ]
  float GetExposure() const; // [ property ]

  ezTagSet m_IncludeTags; // [ property ]
  ezTagSet m_ExcludeTags; // [ property ]

  void ApplySettingsToView(ezView* pView) const;

private:
  void UpdateRenderTargetCamera();
  void ShowStats(ezView* pView);

  void ResourceChangeEventHandler(const ezResourceEvent& e);

  ezEnum<ezCameraUsageHint> m_UsageHint;
  ezEnum<ezCameraMode> m_Mode;
  ezRenderToTexture2DResourceHandle m_hRenderTarget;
  float m_fNearPlane = 0.25f;
  float m_fFarPlane = 1000.0f;
  float m_fPerspectiveFieldOfView = 60.0f;
  float m_fOrthoDimension = 10.0f;
  ezRenderPipelineResourceHandle m_hCachedRenderPipeline;

  float m_fAperture = 1.0f;
  ezTime m_ShutterTime = ezTime::Seconds(1.0f);
  float m_fISO = 100.0f;
  float m_fExposureCompensation = 0.0f;

  void MarkAsModified();
  void MarkAsModified(ezCameraComponentManager* pCameraManager);

  bool m_bIsModified = false;
  bool m_bShowStats = false;
  bool m_bRenderTargetInitialized = false;

  // -1 for none, 0 to 9 for ALT+Number
  ezInt8 m_iEditorShortcut = -1; // [ property ]

  void ActivateRenderToTexture();
  void DeactivateRenderToTexture();

  ezViewHandle m_hRenderTargetView;
  ezVec2 m_vRenderTargetRectOffset = ezVec2(0.0f);
  ezVec2 m_vRenderTargetRectSize = ezVec2(1.0f);
  ezCamera m_RenderTargetCamera;
  ezHashedString m_sRenderPipeline;
};
