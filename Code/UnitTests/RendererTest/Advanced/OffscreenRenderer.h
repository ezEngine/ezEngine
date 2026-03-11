#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Texture.h>

struct ezOffscreenTest_SharedTexture
{
  EZ_DECLARE_POD_TYPE();
  ezUInt32 m_uiCurrentTextureIndex = 0;
  ezUInt64 m_uiCurrentSemaphoreValue = 0;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezOffscreenTest_SharedTexture)


class ezOffscreenTest_OpenMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOffscreenTest_OpenMsg, ezProcessMessage);

public:
  ezGALTextureCreationDescription m_TextureDesc;
  ezHybridArray<ezGALPlatformSharedHandle, 2> m_TextureHandles;
};

class ezOffscreenTest_CloseMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOffscreenTest_CloseMsg, ezProcessMessage);
};

class ezOffscreenTest_RenderMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOffscreenTest_RenderMsg, ezProcessMessage);

public:
  ezOffscreenTest_SharedTexture m_Texture;
};

class ezOffscreenTest_RenderResponseMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOffscreenTest_RenderResponseMsg, ezProcessMessage);

public:
  ezOffscreenTest_SharedTexture m_Texture;
};

class ezOffscreenRendererTest : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezOffscreenRendererTest();
  ~ezOffscreenRendererTest();

  virtual void Run() override;
  void OnPresent(ezUInt32 uiCurrentTexture, ezUInt64 uiCurrentSemaphoreValue);

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void BeforeCoreSystemsShutdown() override;

  void MessageFunc(const ezIpcProcessMessageProtocol::Event& msg);


private:
  ezLogWriter::HTML m_LogHTML;
  ezGALDevice* m_pDevice = nullptr;
  ezGALSwapChainHandle m_hSwapChain;
  ezShaderResourceHandle m_hScreenShader;

  ezInt64 m_iHostPID = 0;
  ezUniquePtr<ezIpcChannel> m_pChannel;
  ezUniquePtr<ezIpcProcessMessageProtocol> m_pProtocol;

  bool m_bExiting = false;
  ezHybridArray<ezOffscreenTest_RenderMsg, 2> m_RequestedFrames;
};
