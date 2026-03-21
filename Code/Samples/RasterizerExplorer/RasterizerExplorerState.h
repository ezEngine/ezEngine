#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezRasterizerView;
class ezRasterizerOverlayComponent;
class ezWorld;

enum class RasterizerOverlay
{
  None,
  Optimized,
  Generic,
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
  void DrawDebugGeometry();
  void DrawImGuiPanel();

  void CreateOverlayObject();
  void DestroyOverlayObject();
  void UpdateOverlayTexture();
  void SaveSettings();
  void LoadSettings();

  struct Building
  {
    ezTransform m_Transform;
    ezVec3 m_vExtents;
  };

  ezUniquePtr<ezWorld> m_pWorld;
  ezUniquePtr<ezRasterizerView> m_pRasterizerViewOptimized;
  ezUniquePtr<ezRasterizerView> m_pRasterizerViewGeneric;

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
};
