#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

#include <RendererFoundation/Resources/ReadbackHelper.h>

class ezXRInterface;
class ezRenderGraph;

/// \brief XR Window base implementation. Optionally wraps a companion window.
class EZ_GAMEENGINE_DLL ezWindowXR : public ezWindowBase
{
public:
  ezWindowXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> pCompanionWindow);
  virtual ~ezWindowXR();

  virtual ezSizeU32 GetClientAreaSize() const override;

  virtual ezWindowHandle GetNativeWindowHandle() const override;

  virtual bool IsVisible() const override { return true; }
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode) const override;

  virtual void ProcessWindowMessages() override;

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }

  /// \brief Returns the companion window if present.
  const ezWindowBase* GetCompanionWindow() const;

private:
  ezXRInterface* m_pVrInterface = nullptr;
  ezUniquePtr<ezWindowBase> m_pCompanionWindow;
  ezAtomicInteger32 m_iReferenceCount = 0;
};

/// \brief XR Window output target base implementation. Optionally wraps a companion window output target.
class EZ_GAMEENGINE_DLL ezWindowOutputTargetXR : public ezWindowOutputTargetBase
{
public:
  ezWindowOutputTargetXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowOutputTargetGAL> pCompanionWindowOutputTarget);
  ~ezWindowOutputTargetXR();

  virtual void AcquireImage() override {}
  virtual void PresentImage(bool bEnableVSync) override;
  void CompanionViewBeginFrame(bool bThrottleCompanionView = true);
  virtual ezResult StartCaptureImage() override;
  virtual ezEnum<ezCaptureImageResult> WaitCaptureImage(ezImage& out_image) override;

  /// \brief Returns the companion window output target if present.
  const ezWindowOutputTargetBase* GetCompanionWindowOutputTarget() const;

private:
  void RenderCompanionView();
  void OnGALEvent(const ezGALDeviceEvent& e);

  ezXRInterface* m_pXrInterface = nullptr;
  ezTime m_LastPresent;
  ezUniquePtr<ezWindowOutputTargetGAL> m_pCompanionWindowOutputTarget;
  ezConstantBufferStorageHandle m_hCompanionConstantBuffer;
  ezShaderResourceHandle m_hCompanionShader;
  bool m_bRender = false;
  ezSharedPtr<ezRenderGraph> m_pRenderGraph;
  bool m_bCaptureRequested = false;
  bool m_bCaptureInFlight = false;
  ezGALReadbackTextureHelper m_Readback;
  ezGALTextureCreationDescription m_CaptureBackbufferDesc;
};
