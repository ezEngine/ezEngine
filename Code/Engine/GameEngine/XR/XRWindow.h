#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/ActorSystem/ActorPluginWindow.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class ezXRInterface;

/// \brief XR Window base implementation. Optionally wraps a companion window.
class EZ_GAMEENGINE_DLL ezWindowXR : public ezWindowBase
{
public:
  ezWindowXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> pCompanionWindow);
  virtual ~ezWindowXR();

  virtual ezSizeU32 GetClientAreaSize() const override;

  virtual ezWindowHandle GetNativeWindowHandle() const override;

  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode) const override;

  virtual void ProcessWindowMessages() override;

  /// \brief Returns the companion window if present.
  const ezWindowBase* GetCompanionWindow() const;

private:
  ezXRInterface* m_pVrInterface = nullptr;
  ezUniquePtr<ezWindowBase> m_pCompanionWindow;
};

/// \brief XR Window output target base implementation. Optionally wraps a companion window output target.
class EZ_GAMEENGINE_DLL ezWindowOutputTargetXR : public ezWindowOutputTargetBase
{
public:
  ezWindowOutputTargetXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowOutputTargetBase> pCompanionWindowOutputTarget);
  ~ezWindowOutputTargetXR();

  virtual void Present(bool bEnableVSync) override;
  virtual ezResult CaptureImage(ezImage& out_Image) override;

  /// \brief Returns the companion window output target if present.
  const ezWindowOutputTargetBase* GetCompanionWindowOutputTarget() const;

private:
  ezXRInterface* m_pXrInterface = nullptr;
  ezUniquePtr<ezWindowOutputTargetBase> m_pCompanionWindowOutputTarget;
  ezGALTextureHandle m_hCompanionRenderTarget;
  ezConstantBufferStorageHandle m_hCompanionConstantBuffer;
  ezShaderResourceHandle m_hCompanionShader;
};

/// \brief XR actor plugin window base implementation. Optionally wraps a companion window and output target.
class EZ_GAMEENGINE_DLL ezActorPluginWindowXR : public ezActorPluginWindow
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorPluginWindowXR, ezActorPluginWindow);

public:
  ezActorPluginWindowXR(
    ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> companionWindow, ezUniquePtr<ezWindowOutputTargetBase> companionWindowOutput);
  ~ezActorPluginWindowXR();
  void Initialize();

  virtual ezWindowBase* GetWindow() const override;
  virtual ezWindowOutputTargetBase* GetOutputTarget() const override;

protected:
  virtual void Update() override;

private:
  ezXRInterface* m_pVrInterface = nullptr;
  ezUniquePtr<ezWindowXR> m_pWindow;
  ezUniquePtr<ezWindowOutputTargetXR> m_pWindowOutputTarget;
};
