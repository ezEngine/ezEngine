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

/// \brief Adds a camera to the scene.
///
/// Cameras have different use cases which are selected through the ezCameraUsageHint property.
/// A game needs (exactly) one camera with the usage hint "MainView", since that is what the renderer uses to render the output.
/// Other cameras are optional or for specialized use cases.
///
/// The camera component defines the field-of-view, near and far clipping plane distances,
/// which render pipeline to use, which objects to include and exclude in the rendered image and various other options.
///
/// A camera object may be created and controlled through a player prefab, for example in a first person or third person game.
/// It may also be created by an ezGameState and controlled by its game logic, for example in top-down games that don't
/// really have a player object.
///
/// Ultimately camera components don't have functionality, they mostly exist and store some data.
/// It is the game state's decision how the game camera works. By default, the game state iterates over all camera components
/// and picks the best one (usually the "MainView") to place the renderer camera.
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

  /// \brief Sets what the camera should be used for.
  void SetUsageHint(ezEnum<ezCameraUsageHint> val);                      // [ property ]
  ezEnum<ezCameraUsageHint> GetUsageHint() const { return m_UsageHint; } // [ property ]

  /// \brief Sets the asset name (or path) to a render target resource, in case this camera should render to texture.
  void SetRenderTargetFile(ezStringView sFile); // [ property ]
  ezStringView GetRenderTargetFile() const;     // [ property ]

  /// \brief An offset to render only to a part of a texture.
  void SetRenderTargetRectOffset(ezVec2 value);                                  // [ property ]
  ezVec2 GetRenderTargetRectOffset() const { return m_vRenderTargetRectOffset; } // [ property ]

  /// \brief A size to render only to a part of a texture.
  void SetRenderTargetRectSize(ezVec2 value);                                // [ property ]
  ezVec2 GetRenderTargetRectSize() const { return m_vRenderTargetRectSize; } // [ property ]

  /// \brief Specifies whether the camera should be perspective or orthogonal and how to use the aspect ratio.
  void SetCameraMode(ezEnum<ezCameraMode> val);                 // [ property ]
  ezEnum<ezCameraMode> GetCameraMode() const { return m_Mode; } // [ property ]

  /// \brief Configures the distance of the near plane. Objects in front of the near plane get culled and clipped.
  void SetNearPlane(float fVal);                      // [ property ]
  float GetNearPlane() const { return m_fNearPlane; } // [ property ]

  /// \brief Configures the distance of the far plane. Objects behin the far plane get culled and clipped.
  void SetFarPlane(float fVal);                     // [ property ]
  float GetFarPlane() const { return m_fFarPlane; } // [ property ]

  /// \brief Sets the opening angle of the perspective view frustum. Whether this means the horizontal or vertical angle is determined by the camera mode.
  void SetFieldOfView(float fVal);                                   // [ property ]
  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; } // [ property ]

  /// \brief Sets the size of the orthogonal view frustum. Whether this means the horizontal or vertical size is determined by the camera mode.
  void SetOrthoDimension(float fVal);                           // [ property ]
  float GetOrthoDimension() const { return m_fOrthoDimension; } // [ property ]

  /// \brief Returns the handle to the render pipeline that is in use.
  ezRenderPipelineResourceHandle GetRenderPipeline() const;

  /// \brief Returns a handle to the view that the camera renders to.
  ezViewHandle GetRenderTargetView() const;

  /// \brief Sets the name of the render pipeline to use.
  void SetRenderPipelineEnum(const char* szFile);                           // [ property ]
  const char* GetRenderPipelineEnum() const;                                // [ property ]

  void SetAperture(float fAperture);                                        // [ property ]
  float GetAperture() const { return m_fAperture; }                         // [ property ]

  void SetShutterTime(ezTime shutterTime);                                  // [ property ]
  ezTime GetShutterTime() const { return m_ShutterTime; }                   // [ property ]

  void SetISO(float fISO);                                                  // [ property ]
  float GetISO() const { return m_fISO; }                                   // [ property ]

  void SetExposureCompensation(float fEC);                                  // [ property ]
  float GetExposureCompensation() const { return m_fExposureCompensation; } // [ property ]

  float GetEV100() const;                                                   // [ property ]
  float GetExposure() const;                                                // [ property ]

  /// \brief If non-empty, only objects with these tags will be included in this camera's output.
  ezTagSet m_IncludeTags; // [ property ]

  /// \brief If non-empty, objects with these tags will be excluded from this camera's output.
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
  ezTime m_ShutterTime = ezTime::MakeFromSeconds(1.0f);
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
