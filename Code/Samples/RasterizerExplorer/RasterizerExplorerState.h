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
  GenericMOC,
  Diff,

  COUNT
};

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
    bool m_bVisible = false;
  };

  ezUniquePtr<ezWorld> m_pWorld;
  ezSharedPtr<const ezRasterizerObject> m_pGroundObject;
  ezSharedPtr<const ezRasterizerObject> m_pFacadeObject;
  static constexpr ezUInt32 RasterizerCount = static_cast<ezUInt32>(RasterizerOverlay::Diff) - 1;

  /// One view per implementation, matching the overlays from Optimized through GenericMOC.
  ezStaticArray<ezUniquePtr<ezRasterizerView>, RasterizerCount> m_pRasterizerViews;

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

  /// Dimensions and center position of the facade occluder box.
  static constexpr float FacadeWidth = 2.0f;
  static constexpr float FacadeDepth = GroundSize - 9.0f;
  static constexpr float FacadeHeight = 12.0f;
  static constexpr float FacadeCenterX = -31.0f;
  static constexpr float FacadeCenterZ = 5.0f;


  struct PerfTimings
  {
    ezTime m_Last[RasterizerCount];
    ezTime m_Query[RasterizerCount];

    // Smoothed values for display
    double m_fAvgMs[RasterizerCount] = {};
    double m_fAvgQueryMs[RasterizerCount] = {};
  };

  PerfTimings m_Perf;
};
