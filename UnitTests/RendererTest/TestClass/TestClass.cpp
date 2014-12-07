#include <PCH.h>
#include "TestClass.h"
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageUtils.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/MemoryTracker.h>


ezGraphicsTest::ezGraphicsTest()
{
  m_pWindow = nullptr;
  m_pDevice = nullptr;
  m_uiFrameCounter = 0;
}

ezResult ezGraphicsTest::InitializeSubTest(ezInt32 iIdentifier)
{
  // initialize everything up to 'core'
  ezStartup::StartupCore();
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::DeInitializeSubTest(ezInt32 iIdentifier)
{
  // shut down completely
  ezStartup::ShutdownCore();
  ezMemoryTracker::DumpMemoryLeaks();
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::SetupRenderer(ezUInt32 uiResolutionX, ezUInt32 uiResolutionY)
{
  m_uiFrameCounter = 0;

  {
    ezStringBuilder sReadDir = ezTestFramework::GetInstance()->GetAbsOutputPath();
    ezString sFolderName = sReadDir.GetFileName();
    sReadDir.AppendPath("../../../Shared/UnitTests", sFolderName, "ImgCompare");

    ezStringBuilder sWriteDir = ezTestFramework::GetInstance()->GetAbsOutputPath();
    sWriteDir.AppendPath("ImgCompare");

    if (ezOSFile::CreateDirectoryStructure(sWriteDir.GetData()).Failed())
      return EZ_FAILURE;

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    if (ezFileSystem::AddDataDirectory(sWriteDir.GetData(), ezFileSystem::AllowWrites, "ImageComparisonDataDir").Failed())
      return EZ_FAILURE;

    if (ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::ReadOnly, "ImageComparisonDataDir").Failed())
      return EZ_FAILURE;
  }

  // Create a window for rendering
  ezWindowCreationDesc WindowCreationDesc;
  WindowCreationDesc.m_ClientAreaSize.width = uiResolutionX;
  WindowCreationDesc.m_ClientAreaSize.height = uiResolutionY;
  m_pWindow = EZ_DEFAULT_NEW(ezWindow)();
  if (m_pWindow->Initialize(WindowCreationDesc).Failed())
    return EZ_FAILURE;

  // Create a device
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = true;
  DeviceInit.m_bDebugDevice = true;
  DeviceInit.m_PrimarySwapChainDescription.m_pWindow = m_pWindow;
  DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = ezGALMSAASampleCount::None;
  DeviceInit.m_PrimarySwapChainDescription.m_bCreateDepthStencilBuffer = true;
  DeviceInit.m_PrimarySwapChainDescription.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
  DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;
  DeviceInit.m_PrimarySwapChainDescription.m_bVerticalSynchronization = true;

  m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11)(DeviceInit);

  if (m_pDevice->Init().Failed())
    return EZ_FAILURE;

  ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
  const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);
  EZ_ASSERT(pPrimarySwapChain != nullptr, "Failed to init swapchain");

  m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();

  ezGALDevice::SetDefaultDevice(m_pDevice);

  ezGALRasterizerStateCreationDescription RasterStateDesc;
  RasterStateDesc.m_bWireFrame = true;
  RasterStateDesc.m_CullMode = ezGALCullMode::Back;
  RasterStateDesc.m_bFrontCounterClockwise = true;
  m_hRasterizerState = m_pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");


  ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
  DepthStencilStateDesc.m_bDepthTest = true;
  DepthStencilStateDesc.m_bDepthWrite = true;
  m_hDepthStencilState = m_pDevice->CreateDepthStencilState(DepthStencilStateDesc);
  EZ_ASSERT(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");

  return EZ_SUCCESS;
}

void ezGraphicsTest::ShutdownRenderer()
{
  if (m_pDevice)
  {
    m_pDevice->DestroyDepthStencilState(m_hDepthStencilState);
    m_pDevice->DestroyRasterizerState(m_hRasterizerState);

    m_pDevice->Shutdown();
    EZ_DEFAULT_DELETE(m_pDevice);
  }

  if (m_pWindow)
  {
    m_pWindow->Destroy();
    EZ_DEFAULT_DELETE(m_pWindow);
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");
}

void ezGraphicsTest::BeginFrame()
{
  m_pDevice->BeginFrame();

  ezGALContext* pContext = m_pDevice->GetPrimaryContext();

  pContext->SetRasterizerState(m_hRasterizerState);
  pContext->SetDepthStencilState(m_hDepthStencilState);
  //pContext->SetResourceView(ezGALShaderStage::PixelShader, 0, m_hTexView);
  //pContext->SetSamplerState(ezGALShaderStage::PixelShader, 0, m_hSamplerState);

}

void ezGraphicsTest::EndFrame(bool bImageComparison)
{
  m_pWindow->ProcessWindowMessages();

  m_pDevice->Present(m_pDevice->GetPrimarySwapChain());

  m_pDevice->EndFrame();

  if (bImageComparison)
  {
    ezImage img, imgSmall;
    GetScreenshot(img);

    ezStringBuilder sImgName;
    ezTestFramework* pFramework = ezTestFramework::GetInstance();

    const char* szTestName = pFramework->GetTest(pFramework->GetCurrentTestIndex())->m_szTestName;
    const char* szSubTestName = pFramework->GetTest(pFramework->GetCurrentTestIndex())->m_SubTests[pFramework->GetCurrentSubTestIndex()].m_szSubTestName;

    ezImageUtils::ScaleDownHalf(img, imgSmall);

    sImgName.Format("%s_%s_%03u.tga", szTestName, szSubTestName, m_uiFrameCounter);

    imgSmall.SaveTo(sImgName);

    ezImage imgExp, imgExpRGBA;
    ezResult res = imgExp.LoadFrom(sImgName);

    EZ_TEST_BOOL_MSG(res.Succeeded(), "Could not read comparison image '%s'", sImgName.GetData());

    if (res.Succeeded())
    {
      ezImageConversionBase::Convert(imgExp, imgExpRGBA, ezImageFormat::R8G8B8A8_UNORM);

      ezImage imgDiff;
      ezImageUtils::ComputeImageDifferenceABS(imgExpRGBA, imgSmall, imgDiff);

      const ezUInt32 uiError = ezImageUtils::ComputeMeanSquareError(imgDiff, 4);

      EZ_TEST_BOOL_MSG(uiError < 1, "Image Mean-Square Error was %u", uiError);
    }
  }

  ++m_uiFrameCounter;
}

void ezGraphicsTest::GetScreenshot(ezImage& img)
{
  ezGALContext* pContext = m_pDevice->GetPrimaryContext();

  ezGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_pDevice->GetPrimarySwapChain())->GetBackBufferTexture();
  pContext->ReadbackTexture(hBBTexture);

  img.SetWidth(m_pWindow->GetClientAreaSize().width);
  img.SetHeight(m_pWindow->GetClientAreaSize().height);
  img.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  img.AllocateImageData();

  ezGALSystemMemoryDescription MemDesc;
  MemDesc.m_pData = img.GetDataPointer<ezUInt8>();
  MemDesc.m_uiRowPitch = 4 * m_pWindow->GetClientAreaSize().width;
  MemDesc.m_uiSlicePitch = 4 * m_pWindow->GetClientAreaSize().width * m_pWindow->GetClientAreaSize().height;

  ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

  pContext->CopyTextureReadbackResult(hBBTexture, &SysMemDescs);
}

void ezGraphicsTest::ClearScreen(const ezColor& color)
{
  ezGALContext* pContext = m_pDevice->GetPrimaryContext();

  ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
  const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);

  ezGALRenderTargetConfigHandle hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();

  pContext->SetRenderTargetConfig(hBBRT);
  pContext->SetViewport(0.0f, 0.0f, (float) m_pWindow->GetClientAreaSize().width, (float) m_pWindow->GetClientAreaSize().height, 0.0f, 1.0f);
  pContext->Clear(color);


}




