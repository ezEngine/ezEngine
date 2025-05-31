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
  const ezGALDeviceCapabilities& caps = GetDeviceCapabilities();

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
        if (caps.m_FormatSupport[i].AreAllSet(ezGALResourceFormatSupport::Texture | ezGALResourceFormatSupport::RenderTarget))
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
}

ezResult ezRendererTestReadback::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bReadbackInProgress = true;

  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  m_hUVColorShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackFloat.ezShader");
  m_hUVColorIntShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackInt.ezShader");
  m_hUVColorUIntShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackUInt.ezShader");
  m_hUVColorDepthShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackDepth.ezShader");

  m_hTexture2DShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
  m_hTexture2DDepthShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2DReadbackDepth.ezShader");
  m_hTexture2DIntShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2DReadbackInt.ezShader");
  m_hTexture2DUIntShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2DReadbackUInt.ezShader");

  // Texture2D
  {
    m_Format = (ezGALResourceFormat::Enum)iIdentifier;
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, m_Format, ezGALMSAASampleCount::None);
    m_hTexture2DReadback = m_pDevice->CreateTexture(desc);

    EZ_ASSERT_DEBUG(!m_hTexture2DReadback.IsInvalidated(), "Failed to create readback texture");
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestReadback::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_Readback.Reset();
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
  m_hUVColorUIntShader.Invalidate();
  m_hTexture2DShader.Invalidate();
  m_hTexture2DDepthShader.Invalidate();
  m_hTexture2DIntShader.Invalidate();
  m_hTexture2DUIntShader.Invalidate();
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
  const bool bIsSigned = ezGALResourceFormat::IsSignedFormat(m_Format);
  if (m_iFrame == 1)
  {
    BeginCommands("Offscreen");
    {
      ezGALRenderingSetup renderingSetup;

      ezShaderResourceHandle shader;
      if (bIsDepthTexture)
      {
        renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hTexture2DReadback));
        renderingSetup.SetClearDepth().SetClearStencil();
        shader = m_hUVColorDepthShader;
      }
      else
      {
        renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DReadback));
        renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);
        if (bIsIntTexture)
        {
          shader = bIsSigned ? m_hUVColorIntShader : m_hUVColorUIntShader;
        }
        else
        {
          shader = m_hUVColorShader;
        }
      }

      ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
      ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
      SetClipSpace();

      ezRenderContext::GetDefaultInstance()->BindShader(shader);
      ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
      ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      ezRenderContext::GetDefaultInstance()->EndRendering();
    }

    // Queue readback
    {
      m_Readback.ReadbackTexture(*m_pEncoder, m_hTexture2DReadback);
    }

    // Wait for results
    {
      ezEnum<ezGALAsyncResult> res = m_Readback.GetReadbackResult(ezTime::MakeFromHours(1));
      EZ_ASSERT_ALWAYS(res == ezGALAsyncResult::Ready, "Readback of texture failed");
    }

    // Readback result
    {
      m_bReadbackInProgress = false;

      const ezGALTexture* pBackbuffer = ezGALDevice::GetDefaultDevice()->GetTexture(m_hTexture2DReadback);
      ezImage readBackResult;
      {
        ezGALTextureSubresource sourceSubResource;
        ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
        ezHybridArray<ezGALSystemMemoryDescription, 1> memory;
        ezReadbackTextureLock lock = m_Readback.LockTexture(sourceSubResources, memory);
        EZ_ASSERT_ALWAYS(lock, "Failed to lock readback texture");
        ezTextureUtils::CopySubResourceToImage(pBackbuffer->GetDescription(), sourceSubResource, memory[0], readBackResult, false);
      }

      {
        ezGALSystemMemoryDescription MemDesc;
        MemDesc.m_pData = readBackResult.GetByteBlobPtr();
        MemDesc.m_uiRowPitch = static_cast<ezUInt32>(readBackResult.GetRowPitch());
        MemDesc.m_uiSlicePitch = static_cast<ezUInt32>(readBackResult.GetDepthPitch());
        ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

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
    }
    EndCommands();
  }

  BeginCommands("Readback");
  {
    if (bIsDepthTexture || ezGALResourceFormat::IsFloatFormat(m_Format))
    {
      m_hShader = m_hTexture2DDepthShader;
    }
    else if (bIsIntTexture)
    {
      m_hShader = ezGALResourceFormat::IsSignedFormat(m_Format) ? m_hTexture2DIntShader : m_hTexture2DUIntShader;
    }
    else
    {
      m_hShader = m_hTexture2DShader;
    }

    {
      ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
      RenderCube(viewport, mMVP, 0xFFFFFFFF, m_pDevice->GetDefaultResourceView(m_hTexture2DReadback));
    }
    if (!m_bReadbackInProgress)
    {
      ezRectFloat viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);

      ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0, &viewport);
      ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_pDevice->GetDefaultResourceView(m_hTexture2DUpload));
      RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);
      EndRendering();
      CompareUploadImage();
    }
  }
  EndCommands();
  return m_bReadbackInProgress ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

static ezRendererTestReadback g_ReadbackTest;
