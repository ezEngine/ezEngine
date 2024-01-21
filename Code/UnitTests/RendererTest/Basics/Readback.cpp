#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererTest/Basics/Readback.h>

void ezRendererTestReadback::SetupSubTests()
{
  ezStartup::StartupCoreSystems();
  SetupRenderer().AssertSuccess();

  const ezGALDeviceCapabilities& caps = ezGALDevice::GetDefaultDevice()->GetCapabilities();

  m_TestableFormats.Clear();
  for (ezUInt32 i = 1; i < ezGALResourceFormat::ENUM_COUNT; i++)
  {
    switch (i)
    {
      case ezGALResourceFormat::AUByteNormalized:      // What use is this format over RUByteNormalized?
      case ezGALResourceFormat::RGB10A2UInt:           // no ezImage support
      case ezGALResourceFormat::RGB10A2UIntNormalized: // no ezImage support
      case ezGALResourceFormat::D24S8:                 // no stencil readback implemented in Vulkan
        break;

      default:
      {
        if (caps.m_FormatSupport[i].AreAllSet(ezGALResourceFormatSupport::Sample | ezGALResourceFormatSupport::Render))
        {
          m_TestableFormats.PushBack((ezGALResourceFormat::Enum)i);
        }
      }
    }
  }

  m_TestableFormatStrings.Reserve(m_TestableFormats.GetCount());
  for (ezGALResourceFormat::Enum format : m_TestableFormats)
  {
    ezStringBuilder sFormat;
    ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezGALResourceFormat>(), format, sFormat, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
    m_TestableFormatStrings.PushBack(sFormat);
    AddSubTest(m_TestableFormatStrings.PeekBack(), format);
  }

  ShutdownRenderer();
  ezStartup::ShutdownCoreSystems();
}

ezResult ezRendererTestReadback::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bReadbackInProgress = true;

  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  m_hUVColorShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackFloat.ezShader");
  m_hUVColorIntShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackInt.ezShader");
  m_hUVColorDepthShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackDepth.ezShader");

  m_hTexture2DShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
  m_hTexture2DIntShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2DReadbackInt.ezShader");

  // Texture2D
  {
    m_Format = (ezGALResourceFormat::Enum)iIdentifier;
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, m_Format,
      ezGALMSAASampleCount::None);
    desc.m_ResourceAccess.m_bReadBack = true;
    m_hTexture2DReadback = m_pDevice->CreateTexture(desc);

    EZ_ASSERT_DEBUG(!m_hTexture2DReadback.IsInvalidated(), "Failed to create readback texture");
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestReadback::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_ReadBackResult.Clear();
  if (!m_hTexture2DReadback.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DReadback);
    m_hTexture2DReadback.Invalidate();
  }
  if (!m_hTexture2DUpload.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DUpload);
    m_hTexture2DUpload.Invalidate();
  }
  m_hShader.Invalidate();
  m_hUVColorShader.Invalidate();
  m_hUVColorIntShader.Invalidate();
  m_hTexture2DShader.Invalidate();
  m_hTexture2DIntShader.Invalidate();
  m_hUVColorDepthShader.Invalidate();

  DestroyWindow();

  if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezRendererTestReadback::GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber)
{
  if (m_ReadBackResult.IsValid())
  {
    ref_img.ResetAndCopy(m_ReadBackResult);
    m_ReadBackResult.Clear();
    return EZ_SUCCESS;
  }

  return SUPER::GetImage(ref_img, subTest, uiImageNumber);
}


void ezRendererTestReadback::MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const
{
  if (!m_sReadBackReferenceImage.IsEmpty())
  {
    out_sString = m_sReadBackReferenceImage;
    m_sReadBackReferenceImage.Clear();
    return;
  }

  return SUPER::MapImageNumberToString(szTestName, subTest, uiImageNumber, out_sString);
}

void ezRendererTestReadback::CompareReadbackImage(ezImage&& image)
{
  ezStringBuilder sTemp;
  ezUInt8 uiChannels = ezGALResourceFormat::GetChannelCount(m_Format);
  if (ezGALResourceFormat::IsDepthFormat(m_Format))
  {
    sTemp = "Readback_Depth";
  }
  else
  {
    sTemp.SetFormat("Readback_Color{}Channel", uiChannels);
  }
  m_sReadBackReferenceImage = sTemp;
  m_ReadBackResult.ResetAndMove(std::move(image));

  EZ_TEST_IMAGE(0, 1);
}

void ezRendererTestReadback::CompareUploadImage()
{
  ezStringBuilder sTemp;
  ezUInt8 uiChannels = ezGALResourceFormat::GetChannelCount(m_Format);
  if (ezGALResourceFormat::IsDepthFormat(m_Format))
  {
    sTemp = "Readback_Upload_Depth";
  }
  else
  {
    sTemp.SetFormat("Readback_Upload_Color{}Channel", uiChannels);
  }
  m_sReadBackReferenceImage = sTemp;
  EZ_TEST_IMAGE(1, 3);
}

ezTestAppRun ezRendererTestReadback::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  BeginFrame();
  ezTestAppRun res = Readback(uiInvocationCount);
  EndFrame();
  return res;
}


ezTestAppRun ezRendererTestReadback::Readback(ezUInt32 uiInvocationCount)
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  const bool bIsDepthTexture = ezGALResourceFormat::IsDepthFormat(m_Format);
  const bool bIsIntTexture = ezGALResourceFormat::IsIntegerFormat(m_Format);
  if (m_iFrame == 1)
  {
    BeginPass("Offscreen");
    {
      ezGALRenderingSetup renderingSetup;

      ezShaderResourceHandle shader;
      if (bIsDepthTexture)
      {
        renderingSetup.m_bDiscardDepth = true;
        renderingSetup.m_bClearDepth = true;
        renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hTexture2DReadback));
        shader = m_hUVColorDepthShader;
      }
      else
      {
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DReadback));
        shader = bIsIntTexture ? m_hUVColorIntShader : m_hUVColorShader;
      }
      renderingSetup.m_ClearColor = ezColor::RebeccaPurple;
      renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

      ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
      ezGALRenderCommandEncoder* pCommandEncoder = ezRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
      SetClipSpace();

      ezRenderContext::GetDefaultInstance()->BindShader(shader);
      ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
      ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      ezRenderContext::GetDefaultInstance()->EndRendering();
    }

    // Queue readback
    {
      auto pCommandEncoder = m_pPass->BeginRendering(ezGALRenderingSetup());
      pCommandEncoder->ReadbackTexture(m_hTexture2DReadback);
      m_pPass->EndRendering(pCommandEncoder);
    }

    // Readback result
    {
      m_bReadbackInProgress = false;
      auto pCommandEncoder = m_pPass->BeginRendering(ezGALRenderingSetup());

      const ezGALTexture* pBackbuffer = ezGALDevice::GetDefaultDevice()->GetTexture(m_hTexture2DReadback);
      const ezEnum<ezGALResourceFormat> format = pBackbuffer->GetDescription().m_Format;

      ezImageHeader header;
      header.SetWidth(pBackbuffer->GetDescription().m_uiWidth);
      header.SetHeight(pBackbuffer->GetDescription().m_uiHeight);
      header.SetImageFormat(ezTextureUtils::GalFormatToImageFormat(format, false));
      ezImage readBackResult;
      readBackResult.ResetAndAlloc(header);

      ezGALSystemMemoryDescription MemDesc;
      MemDesc.m_pData = readBackResult.GetPixelPointer<ezUInt8>();
      MemDesc.m_uiRowPitch = readBackResult.GetRowPitch();
      MemDesc.m_uiSlicePitch = readBackResult.GetDepthPitch();

      ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
      ezGALTextureSubresource sourceSubResource;
      ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
      pCommandEncoder->CopyTextureReadbackResult(m_hTexture2DReadback, sourceSubResources, SysMemDescs);

      {
        ezGALTextureCreationDescription desc;
        desc.m_uiWidth = 8;
        desc.m_uiHeight = 8;
        desc.m_Format = m_Format;
        m_hTexture2DUpload = m_pDevice->CreateTexture(desc, SysMemDescs);
        EZ_ASSERT_DEV(!m_hTexture2DUpload.IsInvalidated(), "Texture creation failed");
      }


      EZ_TEST_BOOL(readBackResult.Convert(ezImageFormat::R32G32B32A32_FLOAT).Succeeded());
      if (bIsIntTexture)
      {
        // For int textures, we multiply by 127 in the shader. We reverse this here to make all formats fit into the [0-1] float range.
        const ezImageFormat::Enum imageFormat = readBackResult.GetImageFormat();
        ezUInt64 uiNumElements = ezUInt64(8) * readBackResult.GetByteBlobPtr().GetCount() / (ezUInt64)ezImageFormat::GetBitsPerPixel(imageFormat);
        // Work with single channels instead of pixels
        uiNumElements *= ezImageFormat::GetBitsPerPixel(imageFormat) / 32;

        const ezUInt32 uiStride = 4;
        void* targetPointer = readBackResult.GetByteBlobPtr().GetPtr();
        while (uiNumElements)
        {
          ezUInt8 uiChannels = ezGALResourceFormat::GetChannelCount(m_Format);
          float& pixel = *reinterpret_cast<float*>(targetPointer);
          if (bIsDepthTexture)
          {
            // Don't normalize alpha channel which was added by the format extension from R to RGBA.
            if ((uiNumElements % 4) != 1)
              pixel /= ezMath::MaxValue<ezUInt16>();
          }
          else
          {
            // Don't normalize alpha channel if it was added by the format extension to RGBA.
            if (uiChannels == 4 || ((uiNumElements % 4) != 1))
              pixel /= 127.0f;
          }

          targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, uiStride);
          uiNumElements--;
        }
      }
      CompareReadbackImage(std::move(readBackResult));



      m_pPass->EndRendering(pCommandEncoder);
    }
    EndPass();
  }

  BeginPass("Readback");
  {
    m_hShader = (bIsIntTexture && !bIsDepthTexture) ? m_hTexture2DIntShader : m_hTexture2DShader;
    {
      ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
      RenderCube(viewport, mMVP, 0xFFFFFFFF, m_pDevice->GetDefaultResourceView(m_hTexture2DReadback));
    }
    if (!m_bReadbackInProgress)
    {
      ezRectFloat viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);

      ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0, &viewport);
      ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_pDevice->GetDefaultResourceView(m_hTexture2DUpload));
      RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);
      CompareUploadImage();
      EndRendering();
    }
  }
  EndPass();
  return m_bReadbackInProgress ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

static ezRendererTestReadback g_ReadbackTest;
