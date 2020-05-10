#pragma once

#include <GameEngine/ActorSystem/ActorPluginWindow.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Shader/ShaderResource.h>

class ezXRInterface;

class EZ_GAMEENGINE_DLL ezWindowXR : public ezWindowBase
{
public:
  ezWindowXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> pCompanionWindow);
  virtual ~ezWindowXR();

  virtual ezSizeU32 GetClientAreaSize() const override;

  virtual ezWindowHandle GetNativeWindowHandle() const override;

  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode) const override;

  virtual void ProcessWindowMessages() override;

  ezXRInterface* m_pVrInterface = nullptr;
  ezUniquePtr<ezWindowBase> m_pCompanionWindow;
};

class EZ_GAMEENGINE_DLL ezWindowOutputTargetXR : public ezWindowOutputTargetBase
{
public:
  ezWindowOutputTargetXR(
    ezXRInterface* pVrInterface, ezUniquePtr<ezWindowOutputTargetBase> pCompanionWindowOutputTarget);
  ~ezWindowOutputTargetXR();

  virtual void Present(bool bEnableVSync) override;
  virtual ezResult CaptureImage(ezImage& out_Image) override;

  ezXRInterface* m_pXrInterface = nullptr;
  ezUniquePtr<ezWindowOutputTargetBase> m_pCompanionWindowOutputTarget;
  ezGALTextureHandle m_hCompanionRenderTarget;
  ezConstantBufferStorageHandle m_hCompanionConstantBuffer;
  ezShaderResourceHandle m_hCompanionShader;
};

class EZ_GAMEENGINE_DLL ezActorPluginWindowXR : public ezActorPluginWindow
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorPluginWindowXR, ezActorPluginWindow);

public:
  ezActorPluginWindowXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> companionWindow,
    ezUniquePtr<ezWindowOutputTargetBase> companionWindowOutput);
  ~ezActorPluginWindowXR();
  void Initialize();

  virtual ezWindowBase* GetWindow() const override;
  virtual ezWindowOutputTargetBase* GetOutputTarget() const override;

  ezXRInterface* m_pVrInterface = nullptr;
  ezUniquePtr<ezWindowXR> m_pWindow;
  ezUniquePtr<ezWindowOutputTargetXR> m_pWindowOutputTarget;

protected:
  virtual void Update() override;
};
