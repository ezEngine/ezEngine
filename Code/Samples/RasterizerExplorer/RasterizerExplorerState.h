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

  void GenerateTransforms();
  void RunRasterizers();
  void DrawDebugGeometry();
  void DrawImGuiPanel();
  bool IsCubeVisible(ezUInt32 uiIndex) const;

  void CreateOverlayObject();
  void DestroyOverlayObject();
  void UpdateOverlayTexture();
  void SaveSettings();
  void LoadSettings();

  ezUniquePtr<ezWorld> m_pWorld;
  ezUniquePtr<ezRasterizerView> m_pRasterizerViewOptimized;
  ezUniquePtr<ezRasterizerView> m_pRasterizerViewGeneric;

  ezHybridArray<ezTransform, 128> m_Transforms;
  ezInt32 m_iSelectedCube = -1;
  bool m_bCullNearby = false;
  float m_fCullDistance = 5.0f;

  RasterizerOverlay m_Overlay = RasterizerOverlay::None;
  ezGALTextureHandle m_hOverlayTexture;
  ezGameObjectHandle m_hOverlayObject;
  ezComponentHandle m_hOverlayComponent;
  ezDynamicArray<ezColorLinearUB> m_OverlayBuffer;

  static constexpr ezUInt32 NumBoxes = 100;
  static constexpr ezUInt32 RasterizerSize = 512;
};
