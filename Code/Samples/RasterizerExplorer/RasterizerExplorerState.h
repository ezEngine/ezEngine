#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezRasterizerView;
class ezRasterizerOverlayComponent;
class ezWorld;

enum class RasterizerOverlay
{
  None,
  Optimized,
  MOC_SSE2,
  MOC_SSE41,
  MOC_AVX2,
  MOC_AVX512,
  GenericMOC,
  Diff,

  COUNT
};

/// Number of MaskedOcclusionCulling instruction set variants, matching ezMOCInstructionSets.
static constexpr ezUInt32 MOCVariantCount = 4;

class RasterizerExplorerState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(RasterizerExplorerState, ezGameState);

public:
  RasterizerExplorerState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;
  virtual void SetupMainView(ezGALSwapChainHandle hSwapChain, ezSizeU32 viewportSize) override;

private:
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;

  void GenerateCity();
  void RunRasterizers();
  ezRasterizerView* GetOverlayView(RasterizerOverlay overlay) const;
  ezRasterizerView* GetMOCView(ezUInt32 uiMOCIndex) const;
  void DrawDebugGeometry();
  void DrawImGuiPanel();
  void DrawPerfPanel();

  void CreateOverlayObject();
  void DestroyOverlayObject();
  void UpdateOverlayTexture();
  void SaveSettings();
  void LoadSettings();

  struct Building
  {
    ezTransform m_Transform;
    ezVec3 m_vExtents;
    ezSharedPtr<const ezRasterizerObject> m_pRasterizerObject;
  };

  ezUniquePtr<ezWorld> m_pWorld;
  ezSharedPtr<const ezRasterizerObject> m_pGroundObject;
  ezSharedPtr<const ezRasterizerObject> m_pFacadeObject;
  ezUniquePtr<ezRasterizerView> m_pRasterizerViewOptimized;
  ezUniquePtr<ezRasterizerView> m_pRasterizerViewGenericMOC;

  /// One view per MaskedOcclusionCulling instruction set, in the order of ezMOCInstructionSets.
  ezStaticArray<ezUniquePtr<ezRasterizerView>, 4> m_pMOCViews;

  ezHybridArray<Building, 128> m_Buildings;
  ezInt32 m_iSelectedBuilding = -1;
  bool m_bCullNearby = false;
  float m_fCullDistance = 20.0f;

  RasterizerOverlay m_Overlay = RasterizerOverlay::None;
  ezGALTextureHandle m_hOverlayTexture;
  ezGameObjectHandle m_hOverlayObject;
  ezComponentHandle m_hOverlayComponent;
  ezDynamicArray<ezColorLinearUB> m_OverlayBuffer;

  static constexpr ezUInt32 GridSize = 10;
  static constexpr float BlockSize = 4.0f;
  static constexpr float StreetWidth = 2.0f;
  static constexpr float CellSize = BlockSize + StreetWidth;
  static constexpr float GroundSize = GridSize * CellSize + StreetWidth;
  static constexpr float GroundHalf = GroundSize * 0.5f;
  static constexpr ezUInt32 RasterizerSize = 512;
  static constexpr ezUInt32 BenchmarkResolution = 1024;
  static constexpr ezUInt32 BenchmarkWarmupFrames = 2;
  static constexpr ezUInt32 BenchmarkSampleFrames = 5;
  static constexpr ezUInt32 BenchmarkImplementationCount = MOCVariantCount + 2;
  static constexpr ezUInt32 BenchmarkQueryIterations = 1000;

  struct PerfTimings
  {
    ezTime m_LastOptimized;
    ezTime m_LastGenericMOC;

    // Smoothed values for display
    double m_fAvgOptimizedMs = 0.0;
    double m_fAvgGenericMOCMs = 0.0;

    /// Timings of the MaskedOcclusionCulling variants, indexed like ezMOCInstructionSets.
    ezTime m_LastMOC[MOCVariantCount] = {};
    double m_fAvgMOCMs[MOCVariantCount] = {};

    double m_fBenchmarkRasterizationMs[BenchmarkImplementationCount] = {};
    double m_fBenchmarkQueryMs[BenchmarkImplementationCount] = {};
    float m_fBenchmarkMinimumShrink[BenchmarkImplementationCount] = {};
  };

  PerfTimings m_Perf;
  ezUInt32 m_uiBenchmarkFrame = 0;
  bool m_bBenchmarkQueriesPrepared = false;
};
