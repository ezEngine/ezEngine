#include "Application.h"
#include "Window.h"
#include "MainRenderPass.h"
#include <Core/Input/InputManager.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Logging/Log.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <RendererDX11/Device/DeviceDX11.h>
typedef ezGALDeviceDX11 ezGALDeviceDefault;
#endif

#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Material/MaterialResource.h>

void SampleApp::InitRendering()
{
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = true;
  DeviceInit.m_bDebugDevice = true;
  DeviceInit.m_PrimarySwapChainDescription.m_pWindow = m_pWindow;
  DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = ezGALMSAASampleCount::None;
  DeviceInit.m_PrimarySwapChainDescription.m_bCreateDepthStencilBuffer = true;
  DeviceInit.m_PrimarySwapChainDescription.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
  DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;
  DeviceInit.m_PrimarySwapChainDescription.m_bVerticalSynchronization = false;

  ezGALDevice* pDevice = EZ_DEFAULT_NEW(ezGALDeviceDefault)(DeviceInit);
  EZ_VERIFY(pDevice->Init() == EZ_SUCCESS, "Device init failed!");

  ezGALDevice::SetDefaultDevice(pDevice);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezRendererCore::SetShaderPlatform("DX11_SM40", true);
#endif

  m_pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline)(ezRenderPipeline::Asynchronous);
  m_pMainRenderPass = EZ_DEFAULT_NEW(MainRenderPass);
  m_pRenderPipeline->AddPass(m_pMainRenderPass);

  ezSizeU32 size = m_pWindow->GetClientAreaSize();
  m_View.SetViewport(ezRectFloat(0.0f, 0.0f, (float) size.width, (float) size.height));

  m_View.SetRenderPipeline(m_pRenderPipeline);

    // Setup default resources
  {
    ezTextureResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Fallback_D.dds");
    ezTextureResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/MissingTexture_D.dds");
    ezMaterialResourceHandle hMissingMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Missing.ezMaterial");

    ezTextureResource::SetTypeFallbackResource(hFallbackTexture);
    ezTextureResource::SetTypeMissingResource(hMissingTexture);
    ezMaterialResource::SetTypeMissingResource(hMissingMaterial);
  }
}

void SampleApp::DeinitRendering()
{
  EZ_DEFAULT_DELETE(m_pMainRenderPass);

  EZ_DEFAULT_DELETE(m_pRenderPipeline);
}

