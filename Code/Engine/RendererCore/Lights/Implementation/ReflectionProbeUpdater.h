#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

EZ_DECLARE_FLAGS(ezUInt8, ezReflectionProbeUpdaterFlags, SkyLight, HasCustomCubeMap);

/// \brief Renders reflection probes and stores filtered mipmap chains into an atlas texture as well as computing sky irradiance
/// Rendering sky irradiance is optional and only done if m_iIrradianceOutputIndex != -1.
class ezReflectionProbeUpdater
{
public:
  /// \brief Defines the target specular reflection probe atlas and index as well as the sky irradiance atlas and index in case the rendered cube map is a sky light.
  struct TargetSlot
  {
    ezGALTextureHandle m_hSpecularOutputTexture;   ///< Must be a valid cube map texture array handle.
    ezGALTextureHandle m_hIrradianceOutputTexture; ///< Optional. Must be set if m_iIrradianceOutputIndex != -1.
    ezInt32 m_iSpecularOutputIndex = -1;           ///< Must be a valid index into the atlas texture.
    ezInt32 m_iIrradianceOutputIndex = -1;         ///< If -1, no irradiance is computed.
  };

public:
  ezReflectionProbeUpdater();
  ~ezReflectionProbeUpdater();

  /// \brief Returns how many new probes can be started this frame.
  /// \param out_updatesFinished Contains the probes that finished last frame.
  /// \return The number of new probes can be started this frame.
  ezUInt32 GetFreeUpdateSlots(ezDynamicArray<ezReflectionProbeRef>& out_updatesFinished);

  /// \brief Starts rendering a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param globalTransform World position to be rendered.
  /// \param target Where the probe should be rendered into.
  /// \return Returns EZ_FAILURE if no more free slots are available.
  ezResult StartDynamicUpdate(const ezReflectionProbeRef& probe, const ezReflectionProbeDesc& desc, const ezTransform& globalTransform, const TargetSlot& target);

  /// \brief Starts filtering an existing cube map into a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param sourceTexture Cube map that should be filtered into a reflection probe.
  /// \param target Where the probe should be rendered into.
  /// \return Returns EZ_FAILURE if no more free slots are available.
  ezResult StartFilterUpdate(const ezReflectionProbeRef& probe, const ezReflectionProbeDesc& desc, ezTextureCubeResourceHandle hSourceTexture, const TargetSlot& target);

  /// \brief Cancel a previously started update.
  void CancelUpdate(const ezReflectionProbeRef& probe);

  /// \brief Generates update steps. Should be called in PreExtraction phase.
  void GenerateUpdateSteps();

  /// \brief Schedules probe rendering views. Should be called at some point during the extraction phase. Can be called multiple times. It will only do work on the first call after GenerateUpdateSteps.
  void ScheduleUpdateSteps();

private:
  struct ReflectionView
  {
    ezViewHandle m_hView;
    ezCamera m_Camera;
  };

  struct UpdateStep
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      RenderFace0,
      RenderFace1,
      RenderFace2,
      RenderFace3,
      RenderFace4,
      RenderFace5,
      Filter,

      ENUM_COUNT,

      Default = Filter
    };

    static bool IsRenderStep(Enum value) { return value >= UpdateStep::RenderFace0 && value <= UpdateStep::RenderFace5; }
    static Enum NextStep(Enum value) { return static_cast<UpdateStep::Enum>((value + 1) % UpdateStep::ENUM_COUNT); }
  };

  struct ProbeUpdateInfo
  {
    ProbeUpdateInfo();
    ~ProbeUpdateInfo();

    ezBitflags<ezReflectionProbeUpdaterFlags> m_flags;
    ezReflectionProbeRef m_probe;
    ezReflectionProbeDesc m_desc;
    ezTransform m_globalTransform;
    ezTextureCubeResourceHandle m_sourceTexture;
    TargetSlot m_TargetSlot;

    struct Step
    {
      EZ_DECLARE_POD_TYPE();

      ezUInt8 m_uiViewIndex;
      ezEnum<UpdateStep> m_UpdateStep;
    };

    bool m_bInUse = false;
    ezEnum<UpdateStep> m_LastUpdateStep;

    ezHybridArray<Step, 8> m_UpdateSteps;

    ezGALTextureHandle m_hCubemap;
    ezGALTextureHandle m_hCubemapProxies[6];
  };

private:
  static void CreateViews(
    ezDynamicArray<ReflectionView>& views, ezUInt32 uiMaxRenderViews, const char* szNameSuffix, const char* szRenderPipelineResource);
  void CreateReflectionViewsAndResources();

  void ResetProbeUpdateInfo(ezUInt32 uiInfo);
  void AddViewToRender(const ProbeUpdateInfo::Step& step, ProbeUpdateInfo& updateInfo);

  bool m_bUpdateStepsFlushed = true;

  ezDynamicArray<ReflectionView> m_RenderViews;
  ezDynamicArray<ReflectionView> m_FilterViews;

  // Active Dynamic Updates
  ezDynamicArray<ezUniquePtr<ProbeUpdateInfo>> m_DynamicUpdates;
  ezHybridArray<ezReflectionProbeRef, 4> m_FinishedLastFrame;
};
