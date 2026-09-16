#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererTest/Advanced/AdvancedFeatures.h>
#include <RendererTest/Basics/RendererTestUtils.h>
#undef CreateWindow
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <sys/prctl.h>
#endif

namespace
{
  /// One column of the depth bias test. Each column draws a reference quad, then the same quad again pushed away from the viewer by s_uiDepthBiasGapUnits and rasterized with the depth bias under test. The biased quad wins the 'Less' depth test exactly if the bias pulls it back in front of the reference quad.
  struct DepthBiasCase
  {
    const char* m_szName;
    bool m_bSloped;        ///< Whether the quads are tilted, which is what makes the slope scaled bias do anything.
    ezInt32 m_iDepthBias;
    float m_fSlopeScaledDepthBias;
    float m_fClampUnits;   ///< Depth bias clamp, in depth format units. Zero means no clamping.
    bool m_bExpectVisible; ///< Whether the biased quad is expected to pass the depth test.
  };

  constexpr DepthBiasCase s_DepthBiasCases[] = {
    // Without any bias the biased quad is behind the reference quad and must not show up. This is the baseline the other cases are contrasted against.
    {"NoBias", false, 0, 0.0f, 0.0f, false},
    // A negative constant bias is much larger than the gap and pulls the quad in front.
    {"ConstantTowardsViewer", false, -4000, 0.0f, 0.0f, true},
    // The same magnitude in the other direction pushes it further away.
    {"ConstantAwayFromViewer", false, 4000, 0.0f, 0.0f, false},
    // The slope scaled bias is multiplied with the depth slope of the primitive, which is zero for a screen aligned quad.
    {"SlopeScaledOnFlatGeometry", false, 0, -8.0f, 0.0f, false},
    {"SlopedNoBias", true, 0, 0.0f, 0.0f, false},
    // Same but with tilted geometry. Now the non-zero slope makes the slope scaled bias take effect.
    {"SlopedSlopeScaled", true, 0, -4.0f, 0.0f, true},
    {"SlopedConstant", true, -4000, 0.0f, 0.0f, true},
    // The clamp limits the bias to a magnitude smaller than the gap, so the quad stays hidden despite the large constant bias.
    {"ClampBelowGap", false, -4000, 0.0f, -50.0f, false},
    // The same clamp, but now permissive enough to still clear the gap.
    {"ClampAboveGap", false, -4000, 0.0f, -2000.0f, true},
  };

  constexpr ezUInt32 s_uiDepthBiasCaseCount = EZ_ARRAY_SIZE(s_DepthBiasCases);
  constexpr ezUInt32 s_uiDepthBiasCellSize = 32;
  constexpr float s_fDepthBiasBaseDepth = 0.5f;
  /// Depth difference between the reference and the biased quad, in depth format units. Small enough that every 'visible' case clears it by at least a factor of four, large enough that the depth buffer quantization cannot swallow it.
  constexpr float s_fDepthBiasGapUnits = 200.0f;
  /// Depth added per unit of the quad's local x, which spans the full cell. The resulting window space slope is 2 * s / s_uiDepthBiasCellSize.
  constexpr float s_fDepthBiasSlope = 0.15f;

  /// One cell of the conservative rasterization test. Each cell rasterizes a single quad, either one that is too small to contain a pixel center or
  /// one that comfortably covers 8x8 pixels, and counts how many pixels of the cell ended up lit.
  struct ConservativeRasterCase
  {
    const char* m_szName;
    bool m_bSubPixel; ///< Whether the quad is the sub-pixel one. Otherwise it is the 8x8 pixel one.
    bool m_bConservative;
    ezUInt32 m_uiMinLitPixels;
    ezUInt32 m_uiMaxLitPixels;
  };

  constexpr ConservativeRasterCase s_ConservativeRasterCases[] = {
    // Standard rasterization only produces a fragment when the pixel center is covered, which the sub-pixel quad deliberately avoids.
    {"SubPixelNormal", true, false, 0, 0},
    // Conservative (overestimated) rasterization produces a fragment for every pixel the quad touches at all, so the very same quad now shows up.
    {"SubPixelConservative", true, true, 1, 9},
    // A quad that covers whole pixels must be unaffected: overestimation may only ever add coverage, never remove any.
    {"CoveringNormal", false, false, 64, 64},
    {"CoveringConservative", false, true, 64, 100},
  };

  constexpr ezUInt32 s_uiConservativeRasterCaseCount = EZ_ARRAY_SIZE(s_ConservativeRasterCases);
  constexpr ezUInt32 s_uiConservativeRasterCellSize = 16;
  /// Window space rect of the sub-pixel quad within its cell. It lies inside pixel 8 but excludes that pixel's center at 8.5.
  constexpr float s_fConservativeRasterSubPixelMin = 8.05f;
  constexpr float s_fConservativeRasterSubPixelMax = 8.45f;
  /// Window space rect of the covering quad within its cell. The fractional bounds keep the pixel centers 4.5 to 11.5 covered without landing on a
  /// pixel edge, so standard rasterization produces exactly 8x8 fragments regardless of the fill rule.
  constexpr float s_fConservativeRasterCoveringMin = 4.4f;
  constexpr float s_fConservativeRasterCoveringMax = 11.6f;
} // namespace

void ezRendererTestAdvancedFeatures::SetupSubTests()
{
  const ezGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("01 - ReadRenderTarget", SubTests::ST_ReadRenderTarget);
  if (caps.m_bSupportsVSRenderTargetArrayIndex)
  {
    AddSubTest("02 - VertexShaderRenderTargetArrayIndex", SubTests::ST_VertexShaderRenderTargetArrayIndex);
  }
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (caps.m_bSupportsSharedTextures)
  {
    AddSubTest("03 - SharedTexture", SubTests::ST_SharedTexture);
  }
#endif

  if (caps.m_bShaderStageSupported[ezGALShaderStage::HullShader])
  {
    AddSubTest("04 - Tessellation", SubTests::ST_Tessellation);
  }
  if (caps.m_bShaderStageSupported[ezGALShaderStage::ComputeShader])
  {
    AddSubTest("05 - Compute", SubTests::ST_Compute);
  }
  AddSubTest("06 - FloatSampling", SubTests::ST_FloatSampling);
  AddSubTest("07 - ProxyTexture", SubTests::ST_ProxyTexture);
  AddSubTest("08 - Material", SubTests::ST_Material);

  // MSAA support is per-format. We pick the first sample count that the swap chain color format and the depth format both support.
  const auto colorSupport = caps.m_FormatSupport[ezGALResourceFormat::BGRAUByteNormalized];
  const auto depthSupport = caps.m_FormatSupport[ezGALResourceFormat::D24S8];
  if (colorSupport.AreAllSet(ezGALResourceFormatSupport::RenderTarget | ezGALResourceFormatSupport::MSAA4x) && depthSupport.IsSet(ezGALResourceFormatSupport::MSAA4x))
  {
    AddSubTest("09 - MSAAResolve", SubTests::ST_MSAAResolve);
  }
  else if (colorSupport.AreAllSet(ezGALResourceFormatSupport::RenderTarget | ezGALResourceFormatSupport::MSAA2x) && depthSupport.IsSet(ezGALResourceFormatSupport::MSAA2x))
  {
    AddSubTest("09 - MSAAResolve", SubTests::ST_MSAAResolve);
  }

  AddSubTest("10 - ViewFormatOverride", SubTests::ST_ViewFormatOverride);
  AddSubTest("11 - DepthBias", SubTests::ST_DepthBias);
  if (caps.m_bSupportsConservativeRasterization)
  {
    AddSubTest("12 - ConservativeRasterization", SubTests::ST_ConservativeRasterization);
  }
}

ezResult ezRendererTestAdvancedFeatures::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  if (iIdentifier == ST_ReadRenderTarget)
  {
    // Texture2D
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    m_Texture2DRange = {};
    m_Texture2DRange.m_uiMipLevels = 1;
    m_Texture2DRange.m_uiBaseMipLevel = 0;

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
  }

  if (iIdentifier == ST_ProxyTexture)
  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    // Proxy texture
    m_hProxyTexture2D[0] = m_pDevice->CreateProxyTexture(m_hTexture2DArray, 0);
    m_hProxyTexture2D[1] = m_pDevice->CreateProxyTexture(m_hTexture2DArray, 1);

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
    m_hShader3 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor2.ezShader");
  }

  if (iIdentifier == ST_ViewFormatOverride)
  {
    // Rendering through both the UNorm and the sRGB view of one image is only guaranteed for one of the two channel orders, so use whichever the device supports.
    const ezGALDeviceCapabilities& caps = m_pDevice->GetCapabilities();
    auto IsRenderTarget = [&](ezGALResourceFormat::Enum format)
    { return caps.m_FormatSupport[format].IsSet(ezGALResourceFormatSupport::RenderTarget); };

    ezEnum<ezGALResourceFormat> unormFormat;
    if (IsRenderTarget(ezGALResourceFormat::BGRAUByteNormalized) && IsRenderTarget(ezGALResourceFormat::BGRAUByteNormalizedsRGB))
    {
      unormFormat = ezGALResourceFormat::BGRAUByteNormalized;
      m_OverrideSrgbFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    }
    else if (IsRenderTarget(ezGALResourceFormat::RGBAUByteNormalized) && IsRenderTarget(ezGALResourceFormat::RGBAUByteNormalizedsRGB))
    {
      unormFormat = ezGALResourceFormat::RGBAUByteNormalized;
      m_OverrideSrgbFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    }
    else
    {
      EZ_TEST_FAILURE("No suitable format", "Neither the BGRA nor the RGBA UNorm/sRGB pair is supported as a render target, at least one of them has to be.");
      return EZ_FAILURE;
    }

    // Both textures are created as UNorm, the second one is rendered into through an sRGB render target view so the hardware applies the linear -> sRGB transfer function on write and the stored bits differ. Combining both with an sRGB sampled view isolates the write-side and read-side effects of the format override against each other.
    for (ezUInt32 i = 0; i < 2; i++)
    {
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(8, 8, unormFormat, ezGALMSAASampleCount::None);
      m_hOverrideTexture2D[i] = m_pDevice->CreateTexture(desc);

      ezGALRenderTargetViewCreationDescription viewDesc;
      viewDesc.m_hTexture = m_hOverrideTexture2D[i];
      viewDesc.m_OverrideViewFormat = i == 0 ? ezEnum<ezGALResourceFormat>(ezGALResourceFormat::Invalid) : m_OverrideSrgbFormat;

      m_hOverrideRTV[i] = m_pDevice->GetRenderTargetView(viewDesc);
      if (!EZ_TEST_BOOL(!m_hOverrideRTV[i].IsInvalidated()))
        return EZ_FAILURE;
    }

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
  }

  if (iIdentifier == ST_FloatSampling)
  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, ezGALResourceFormat::D16, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackDepth.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/SampleLevel_PointClampBorder.ezShader");

    ezGALSamplerStateCreationDescription samplerDesc;
    samplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MagFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_AddressU = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressV = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressW = ezImageAddressMode::ClampBorder;
    samplerDesc.m_BorderColor = ezColor::White;

    m_hDepthSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(samplerDesc);
  }

  if (iIdentifier == ST_Compute)
  {
    // Texture2D as compute RW target. Note that SRGB and depth formats are not supported by most graphics cards for this purpose.
    ezEnum<ezGALResourceFormat> textureFormat;
    ezGALResourceFormat::Enum formats[] = {ezGALResourceFormat::RGBAFloat, ezGALResourceFormat::BGRAUByteNormalized, ezGALResourceFormat::RGBAUByteNormalized};
    for (auto format : formats)
    {
      if (m_pDevice->GetCapabilities().m_FormatSupport[format].IsSet(ezGALResourceFormatSupport::TextureRW))
      {
        textureFormat = format;
        break;
      }
    }
    if (!EZ_TEST_BOOL(textureFormat != ezGALResourceFormat::Invalid))
      return EZ_FAILURE;

    // We are only rendering to mip map level 4 (8x8).
    // The array and levels and mip size is only here to test sub-resource views and Nvidia bugs (image will be too dark): https://forums.developer.nvidia.com/t/vulkan-driver-bug-regression-in-hlsl-getdimensions-on-rwtexture2darray/315282
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(128, 128, textureFormat, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::UnorderedAccess;
    desc.m_uiArraySize = 2;
    desc.m_uiMipLevelCount = 6;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    m_Texture2DRange = {};
    m_Texture2DRange.m_uiBaseMipLevel = 4;
    m_Texture2DRange.m_uiMipLevels = 1;
    m_Texture2DRange.m_uiBaseArraySlice = 0;
    m_Texture2DRange.m_uiArraySlices = 1;

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColorCompute.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2DReadbackDepth.ezShader");
  }

  if (iIdentifier == ST_VertexShaderRenderTargetArrayIndex)
  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(320 / 2, 240, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Stereo.ezShader");
    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StereoPreview.ezShader");
  }

  if (iIdentifier == ST_Material)
  {
    // Texture Resource
    ezGALTextureCreationDescription galTexDesc;
    galTexDesc.m_uiWidth = 8;
    galTexDesc.m_uiHeight = 8;
    galTexDesc.m_uiMipLevelCount = 1;
    galTexDesc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;

    ezImage coloredMips;
    ezRendererTestUtils::CreateImage(coloredMips, galTexDesc.m_uiWidth, galTexDesc.m_uiHeight, 1, true);

    ezTempHybridArray<ezGALSystemMemoryDescription, 1> initialData;
    initialData.SetCount(galTexDesc.m_uiMipLevelCount);
    for (ezUInt32 m = 0; m < galTexDesc.m_uiMipLevelCount; m++)
    {
      ezGALSystemMemoryDescription& memoryDesc = initialData[m];
      memoryDesc.m_pData = coloredMips.GetSubImageView(m).GetByteBlobPtr();
      memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(coloredMips.GetDepthPitch(m));
    }

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/TestMaterial.ezShader");

    ezGALSamplerStateCreationDescription samplerDesc;
    samplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MagFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_AddressU = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressV = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressW = ezImageAddressMode::ClampBorder;
    samplerDesc.m_BorderColor = ezColor::White;

    ezTexture2DResourceDescriptor texDesc;
    texDesc.m_DescGAL = galTexDesc;
    texDesc.m_InitialContent = initialData;
    texDesc.m_SamplerDesc = samplerDesc;

    m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
    m_hTexture2 = ezResourceManager::CreateResource<ezTexture2DResource>("TestTexture", std::move(texDesc), "A Test Texture");

    // Material
    ezMaterialResourceDescriptor matDesc;
    matDesc.m_hShader = m_hShader;
    matDesc.m_RenderDataCategory = ezDefaultRenderDataCategories::LitOpaque;
    m_sBaseColor.Assign("BaseColor");
    m_sBaseColor2.Assign("BaseColor2");
    matDesc.m_Parameters.PushBack({m_sBaseColor, ezColor::White});
    matDesc.m_Parameters.PushBack({m_sBaseColor2, ezColor::White});

    m_sTexture.Assign("DiffuseTexture");
    matDesc.m_Texture2DBindings.PushBack({m_sTexture, m_hTexture});

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>("TestMaterial", std::move(matDesc), "A Test Material");
  }

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (iIdentifier == ST_SharedTexture)
  {
    ezCVarFloat* pProfilingThreshold = (ezCVarFloat*)ezCVar::FindCVarByName("Profiling.DiscardThresholdMS");
    EZ_ASSERT_DEBUG(pProfilingThreshold, "Profiling.cpp cvar was renamed");
    m_fOldProfilingThreshold = *pProfilingThreshold;
    *pProfilingThreshold = 0.0f;

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");

    const ezStringBuilder pathToSelf = ezCommandLineUtils::GetGlobalInstance()->GetParameter(0);

    ezProcessOptions opt;
    opt.m_sProcess = pathToSelf;

    ezStringBuilder sIPC;
    ezConversionUtils::ToString(ezUuid::MakeUuid(), sIPC);

    ezStringBuilder sPID;
    ezConversionUtils::ToString(ezProcess::GetCurrentProcessID(), sPID);

#  ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    constexpr const char* szDefaultRenderer = "Vulkan";
#  else
    constexpr const char* szDefaultRenderer = "DX11";
#  endif
    ezStringView sRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);

    opt.m_Arguments.PushBack("-offscreen");
    opt.m_Arguments.PushBack("-IPC");
    opt.m_Arguments.PushBack(sIPC);
    opt.m_Arguments.PushBack("-PID");
    opt.m_Arguments.PushBack(sPID);
    opt.m_Arguments.PushBack("-renderer");
    opt.m_Arguments.PushBack(sRendererName);
    opt.m_Arguments.PushBack("-outputDir");
    opt.m_Arguments.PushBack(ezTestFramework::GetInstance()->GetAbsOutputPath());
    m_pOffscreenProcess = EZ_DEFAULT_NEW(ezProcess);

    // Start the IPC server and wait for the "Connecting" state before starting the client process or it will fail to connect.
    m_pChannel = ezIpcChannel::CreatePipeChannel(sIPC, ezIpcChannel::Mode::Server);
    m_pProtocol = EZ_DEFAULT_NEW(ezIpcProcessMessageProtocol, m_pChannel.Borrow());
    m_pProtocol->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezRendererTestAdvancedFeatures::OffscreenProcessMessageFunc, this));
    EZ_SUCCEED_OR_RETURN(m_pChannel->Connect());
    while (m_pChannel->GetConnectionState() != ezIpcChannel::ConnectionState::Connecting)
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));
    }

    EZ_SUCCEED_OR_RETURN(m_pOffscreenProcess->Launch(opt));

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
    // pidfd_getfd which is used to open the shared textures on Linux Vulkan is blocked by Yama ptrace_scope. With this command we allow our child process to ptrace us.
    if (prctl(PR_SET_PTRACER, m_pOffscreenProcess->GetProcessID()) != 0)
    {
      ezLog::Error("prctl command failed with: {}", ezArgErrno(errno));
    }
#  endif

    m_bExiting = false;
    m_uiReceivedTextures = 0;

    m_SharedTextureDesc.SetAsRenderTarget(8, 8, ezGALResourceFormat::BGRAUByteNormalizedsRGB);
    m_SharedTextureDesc.m_Type = ezGALTextureType::Texture2DShared;

    m_SharedTextureQueue.Clear();
    for (ezUInt32 i = 0; i < s_SharedTextureCount; i++)
    {
      m_hSharedTextures[i] = m_pDevice->CreateSharedTexture(m_SharedTextureDesc);
      EZ_TEST_BOOL(!m_hSharedTextures[i].IsInvalidated());
      m_SharedTextureQueue.PushBack({i, 0});
    }

    while (m_pChannel->GetConnectionState() == ezIpcChannel::ConnectionState::Connecting)
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));
      if (m_pOffscreenProcess->GetState() == ezProcessState::Finished)
      {
        ezUInt32 uiExitCode = m_pOffscreenProcess->GetExitCode();
        ezLog::Error("Process exited prematurely with code: {}", uiExitCode);
        return EZ_FAILURE;
      }
    }

    if (m_pChannel->GetConnectionState() != ezIpcChannel::ConnectionState::Connected)
    {
      ezLog::Error("Failed to connect to offscreen process");
      return EZ_FAILURE;
    }

    ezOffscreenTest_OpenMsg msg;
    msg.m_TextureDesc = m_SharedTextureDesc;
    for (auto& hSharedTexture : m_hSharedTextures)
    {
      const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(hSharedTexture);
      if (pSharedTexture == nullptr)
      {
        return EZ_FAILURE;
      }

      msg.m_TextureHandles.PushBack(pSharedTexture->GetSharedHandle());
    }
    m_pProtocol->Send(&msg);
  }
#endif

  if (iIdentifier == ST_Tessellation)
  {
    {
      ezGeometry geom;
      geom.AddStackedSphere(0.5f, 3, 2);

      ezMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      m_hSphereMesh = ezResourceManager::CreateResource<ezMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
    }

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Tessellation.ezShader");
  }

  if (iIdentifier == ST_MSAAResolve)
  {
    const auto colorSupport = m_pDevice->GetCapabilities().m_FormatSupport[ezGALResourceFormat::BGRAUByteNormalized];
    const auto depthSupport = m_pDevice->GetCapabilities().m_FormatSupport[ezGALResourceFormat::D24S8];
    if (colorSupport.IsSet(ezGALResourceFormatSupport::MSAA4x) && depthSupport.IsSet(ezGALResourceFormatSupport::MSAA4x))
      m_MSAASamples = ezGALMSAASampleCount::FourSamples;
    else
      m_MSAASamples = ezGALMSAASampleCount::TwoSamples;

    constexpr ezUInt32 uiW = 64;
    constexpr ezUInt32 uiH = 64;

    {
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(uiW, uiH, ezGALResourceFormat::BGRAUByteNormalized, m_MSAASamples);
      m_hMSAAColor = m_pDevice->CreateTexture(desc);
      EZ_TEST_BOOL(!m_hMSAAColor.IsInvalidated());
    }
    {
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(uiW, uiH, ezGALResourceFormat::D24S8, m_MSAASamples);
      m_hMSAADepthStencil = m_pDevice->CreateTexture(desc);
      EZ_TEST_BOOL(!m_hMSAADepthStencil.IsInvalidated());
    }
    {
      // Resolve target must match the MSAA color format and is read back for verification, so it needs the default usage flags only.
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(uiW, uiH, ezGALResourceFormat::BGRAUByteNormalized, ezGALMSAASampleCount::None);
      m_hMSAAResolveTarget = m_pDevice->CreateTexture(desc);
      EZ_TEST_BOOL(!m_hMSAAResolveTarget.IsInvalidated());
    }

    {
      // A simple full-NDC quad. The stencil shader (StencilColor.ezShader) only reads POSITION.
      ezGeometry geom;
      geom.AddRect(ezVec2(2.0f, 2.0f), 1, 1);

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezMeshVertexStreamType::Position);
      desc.AddStream(ezMeshVertexStreamType::Color0);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      m_hMSAAQuadMesh = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("MSAAResolveQuad", std::move(desc), "MSAAResolveQuad");
    }

    m_hMSAAStencilShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StencilColor.ezShader");
  }

  if (iIdentifier == ST_DepthBias)
  {
    // The depth bias is expressed in multiples of the smallest resolvable difference of the depth format, so only formats with a known fixed-point
    // representation can be used here. For a floating point depth buffer that unit depends on the depth values of the primitive itself.
    const ezGALDeviceCapabilities& caps = m_pDevice->GetCapabilities();
    ezEnum<ezGALResourceFormat> depthFormat;
    if (caps.m_FormatSupport[ezGALResourceFormat::D16].IsSet(ezGALResourceFormatSupport::RenderTarget))
    {
      depthFormat = ezGALResourceFormat::D16;
      m_fDepthBiasUnit = 1.0f / 65535.0f;
    }
    else if (caps.m_FormatSupport[ezGALResourceFormat::D24S8].IsSet(ezGALResourceFormatSupport::RenderTarget))
    {
      depthFormat = ezGALResourceFormat::D24S8;
      m_fDepthBiasUnit = 1.0f / 16777215.0f;
    }
    else
    {
      EZ_TEST_FAILURE("No suitable format", "Neither D16 nor D24S8 is supported as a depth target, at least one of them has to be.");
      return EZ_FAILURE;
    }

    {
      // Linear format so the readback values can be compared without an sRGB conversion.
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiDepthBiasCellSize * s_uiDepthBiasCaseCount, s_uiDepthBiasCellSize, ezGALResourceFormat::BGRAUByteNormalized, ezGALMSAASampleCount::None);
      m_hDepthBiasColor = m_pDevice->CreateTexture(desc);
      if (!EZ_TEST_BOOL(!m_hDepthBiasColor.IsInvalidated()))
        return EZ_FAILURE;
    }
    {
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiDepthBiasCellSize * s_uiDepthBiasCaseCount, s_uiDepthBiasCellSize, depthFormat, ezGALMSAASampleCount::None);
      m_hDepthBiasDepth = m_pDevice->CreateTexture(desc);
      if (!EZ_TEST_BOOL(!m_hDepthBiasDepth.IsInvalidated()))
        return EZ_FAILURE;
    }

    {
      // A full-NDC quad. StencilColor.ezShader only reads POSITION, the depth of each quad comes entirely from its transform.
      ezGeometry geom;
      geom.AddRect(ezVec2(2.0f, 2.0f), 1, 1);

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezMeshVertexStreamType::Position);
      desc.AddStream(ezMeshVertexStreamType::Color0);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      m_hDepthBiasQuadMesh = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("DepthBiasQuad", std::move(desc), "DepthBiasQuad");
    }

    m_hDepthBiasShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StencilColor.ezShader");
  }

  if (iIdentifier == ST_ConservativeRasterization)
  {
    {
      // Linear format so the readback values can be compared without an sRGB conversion.
      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiConservativeRasterCellSize * s_uiConservativeRasterCaseCount, s_uiConservativeRasterCellSize, ezGALResourceFormat::BGRAUByteNormalized, ezGALMSAASampleCount::None);
      m_hConservativeRasterColor = m_pDevice->CreateTexture(desc);
      if (!EZ_TEST_BOOL(!m_hConservativeRasterColor.IsInvalidated()))
        return EZ_FAILURE;
    }

    {
      // A full-NDC quad. StencilColor.ezShader only reads POSITION, the position of each quad comes entirely from its transform.
      ezGeometry geom;
      geom.AddRect(ezVec2(2.0f, 2.0f), 1, 1);

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezMeshVertexStreamType::Position);
      desc.AddStream(ezMeshVertexStreamType::Color0);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      m_hConservativeRasterQuadMesh = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("ConservativeRasterQuad", std::move(desc), "ConservativeRasterQuad");
    }

    m_hConservativeRasterShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StencilColor.ezShader");
  }

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_SharedTexture:
      m_ImgCompFrames.PushBack(100000000);
      break;
    case SubTests::ST_Tessellation:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_Compute:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_FloatSampling:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ProxyTexture:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_Material:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ColorChange);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ColorChange2);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ChangeTexture);
      break;
    case SubTests::ST_ViewFormatOverride:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_MSAAResolve:
    case SubTests::ST_DepthBias:
    case SubTests::ST_ConservativeRasterization:
      // Uses readback for verification
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestAdvancedFeatures::DeInitializeSubTest(ezInt32 iIdentifier)
{
  if (iIdentifier == ST_Tessellation)
  {
    m_hSphereMesh.Invalidate();
  }
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  else if (iIdentifier == ST_SharedTexture)
  {
    EZ_TEST_BOOL(m_pOffscreenProcess->WaitToFinish(ezTime::MakeFromSeconds(5)).Succeeded());
    EZ_TEST_BOOL(m_pOffscreenProcess->GetState() == ezProcessState::Finished);
    EZ_TEST_INT(m_pOffscreenProcess->GetExitCode(), 0);
    m_pOffscreenProcess = nullptr;

    m_pProtocol = nullptr;
    m_pChannel = nullptr;

    for (ezUInt32 i = 0; i < s_SharedTextureCount; i++)
    {
      m_pDevice->DestroySharedTexture(m_hSharedTextures[i]);
    }
    m_SharedTextureQueue.Clear();

    ezStringView sPath = ":imgout/Profiling/sharedTexture.json"_ezsv;
    EZ_TEST_RESULT(ezProfilingUtils::SaveProfilingCapture(sPath));
    ezStringView sPath2 = ":imgout/Profiling/offscreenProfiling.json"_ezsv;
    ezStringView sMergedFile = ":imgout/Profiling/sharedTexturesMerged.json"_ezsv;
    EZ_TEST_RESULT(ezProfilingUtils::MergeProfilingCaptures(sPath, sPath2, sMergedFile));

    ezCVarFloat* pProfilingThreshold = (ezCVarFloat*)ezCVar::FindCVarByName("Profiling.DiscardThresholdMS");
    EZ_ASSERT_DEBUG(pProfilingThreshold, "Profiling.cpp cvar was renamed");
    *pProfilingThreshold = m_fOldProfilingThreshold;
  }
#endif
  else if (iIdentifier == ST_FloatSampling)
  {
    ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hDepthSamplerState);
  }

  if (iIdentifier == ST_ProxyTexture)
  {
    for (ezUInt32 i = 0; i < 2; i++)
    {
      ezGALDevice::GetDefaultDevice()->DestroyProxyTexture(m_hProxyTexture2D[i]);
    }
  }
  if (iIdentifier == ST_ViewFormatOverride)
  {
    for (ezUInt32 i = 0; i < 2; i++)
    {
      m_hOverrideRTV[i].Invalidate();
      m_pDevice->DestroyTexture(m_hOverrideTexture2D[i]);
    }
    m_OverrideSrgbFormat = ezGALResourceFormat::Invalid;
  }
  m_hShader2.Invalidate();
  m_hShader3.Invalidate();

  m_hTexture.Invalidate();
  m_hTexture2.Invalidate();
  m_hMaterial.Invalidate();

  if (iIdentifier == ST_MSAAResolve)
  {
    m_MSAAReadback.Reset();
    m_hMSAAQuadMesh.Invalidate();
    m_hMSAAStencilShader.Invalidate();
    m_pDevice->DestroyTexture(m_hMSAAColor);
    m_pDevice->DestroyTexture(m_hMSAADepthStencil);
    m_pDevice->DestroyTexture(m_hMSAAResolveTarget);
  }

  m_pDevice->DestroyTexture(m_hTexture2D);
  m_pDevice->DestroyTexture(m_hTexture2DArray);

  if (iIdentifier == ST_DepthBias)
  {
    m_DepthBiasReadback.Reset();
    m_hDepthBiasQuadMesh.Invalidate();
    m_hDepthBiasShader.Invalidate();
    m_pDevice->DestroyTexture(m_hDepthBiasColor);
    m_pDevice->DestroyTexture(m_hDepthBiasDepth);
    m_hDepthBiasDepth.Invalidate();
  }

  if (iIdentifier == ST_ConservativeRasterization)
  {
    m_ConservativeRasterReadback.Reset();
    m_hConservativeRasterQuadMesh.Invalidate();
    m_hConservativeRasterShader.Invalidate();
    m_pDevice->DestroyTexture(m_hConservativeRasterColor);
  }

  DestroyWindow();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestAdvancedFeatures::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (iIdentifier == ST_SharedTexture)
  {
    return SharedTexture();
  }
#endif
  if (iIdentifier == ST_Material)
  {
    return Material();
  }

  BeginFrame();

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      ReadRenderTarget();
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      if (!m_pDevice->GetCapabilities().m_bSupportsVSRenderTargetArrayIndex)
        return ezTestAppRun::Quit;
      VertexShaderRenderTargetArrayIndex();
      break;
    case SubTests::ST_Tessellation:
      Tessellation();
      break;
    case SubTests::ST_Compute:
      Compute();
      break;
    case SubTests::ST_FloatSampling:
      FloatSampling();
      break;
    case SubTests::ST_ProxyTexture:
      ProxyTexture();
      break;
    case SubTests::ST_MSAAResolve:
      MSAAResolve();
      break;
    case SubTests::ST_ViewFormatOverride:
      ViewFormatOverride();
      break;
    case SubTests::ST_DepthBias:
      DepthBias();
      break;
    case SubTests::ST_ConservativeRasterization:
      ConservativeRasterization();
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  EndFrame();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return ezTestAppRun::Quit;
  }
  return ezTestAppRun::Continue;
}

void ezRendererTestAdvancedFeatures::ReadRenderTarget()
{
  BeginCommands("Offscreen");
  {
    TransitionTexture(m_hTexture2D, ezGALResourceState::RenderTarget);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2D));
    renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("Texture2D");
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2D, ezGALResourceState::ShaderResource, m_Texture2DRange);
    TransitionTexture(m_hDepthStencilTexture, ezGALResourceState::DepthStencilWrite);

    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, m_Texture2DRange);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::FloatSampling()
{
  BeginCommands("Offscreen");
  {
    TransitionTexture(m_hTexture2DArray, ezGALResourceState::DepthStencilWrite);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.SetClearDepth();

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("FloatSampling");
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2DArray, ezGALResourceState::DepthStencilRead);

    ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroupTest.BindSampler("DepthSampler", m_hDepthSamplerState);
    bindGroupTest.BindTexture("DepthTexture", m_hTexture2DArray);

    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    {
      ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);
      RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);
      EndRendering();
      if (m_ImgCompFrames.Contains(m_iFrame))
      {
        TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
        EZ_TEST_IMAGE(m_iFrame, 100);
      }
    }
  }
  EndCommands();
}


void ezRendererTestAdvancedFeatures::ProxyTexture()
{
  // We render normal pattern to layer 0 and the blue pattern to layer 1.
  BeginCommands("Offscreen");
  for (ezUInt8 i = 0; i < 2; i++)
  {
    TransitionTexture(m_hProxyTexture2D[i], ezGALResourceState::RenderTarget);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hProxyTexture2D[i]));
    renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(i == 0 ? m_hShader2 : m_hShader3);
    ezRenderContext::GetDefaultInstance()->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();

  // Render both layers using proxy texture (2D) and manually created resource view (2DArray).
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("Texture2DProxy");
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2DArray, ezGALResourceState::ShaderResource, {0, 1, 0, 1});
    TransitionTexture(m_hTexture2DArray, ezGALResourceState::ShaderResource, {1, 1, 0, 1});

    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hProxyTexture2D[0]);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hProxyTexture2D[1]);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {0, 1, 0, 1});
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {1, 1, 0, 1});
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::ViewFormatOverride()
{
  // Render the same gradient into two identically created UNorm textures, the second one through an sRGB render target view.
  BeginCommands("Offscreen");
  for (ezUInt32 i = 0; i < 2; i++)
  {
    TransitionTexture(m_hOverrideTexture2D[i], ezGALResourceState::RenderTarget);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_hOverrideRTV[i]);
    renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();

  // Each column combines one of the two textures with one of the two sampled view formats, so the write-side and read-side overrides can be told apart:
  // 0: write UNorm, read UNorm - the raw gradient.
  // 1: write sRGB,  read UNorm - encoded bits sampled raw, brighter.
  // 2: write UNorm, read sRGB  - raw bits decoded on read, darker.
  // 3: write sRGB,  read sRGB  - the encode cancels the decode, so this must match column 0.
  struct Column
  {
    ezUInt32 m_uiTexture;
    ezEnum<ezGALResourceFormat> m_ReadFormat;
  };
  const Column columns[] = {
    {0, ezGALResourceFormat::Invalid},
    {1, ezGALResourceFormat::Invalid},
    {0, m_OverrideSrgbFormat},
    {1, m_OverrideSrgbFormat},
  };

  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = EZ_ARRAY_SIZE(columns);
  const float fElementWidth = fWidth / uiColumns;

  const ezMat4 mMVP = CreateSimpleMVP(fElementWidth / fHeight);
  BeginCommands("ViewFormatOverride");
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
    TransitionTexture(m_hDepthStencilTexture, ezGALResourceState::DepthStencilWrite);
    TransitionTexture(m_hOverrideTexture2D[0], ezGALResourceState::ShaderResource);
    TransitionTexture(m_hOverrideTexture2D[1], ezGALResourceState::ShaderResource);

    for (ezUInt32 i = 0; i < uiColumns; i++)
    {
      if (i == uiColumns - 1)
        m_bCaptureImage = true;

      ezRectFloat viewport = ezRectFloat(fElementWidth * i, 0, fElementWidth, fHeight);
      BeginRendering(ezColor::RebeccaPurple, i == 0 ? 0xFFFFFFFF : 0, &viewport);
      {
        ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
        bindGroup.BindTexture("DiffuseTexture", m_hOverrideTexture2D[columns[i].m_uiTexture], {}, columns[i].m_ReadFormat);
        RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);
      }
      EndRendering();

      if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
      {
        TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
        EZ_TEST_IMAGE(m_iFrame, 100);
      }
    }
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::VertexShaderRenderTargetArrayIndex()
{
  m_bCaptureImage = true;
  const ezMat4 mMVP = CreateSimpleMVP((m_pWindow->GetClientAreaSize().width / 2.0f) / (float)m_pWindow->GetClientAreaSize().height);
  BeginCommands("Offscreen Stereo");
  {
    TransitionTexture(m_hTexture2DArray, ezGALResourceState::RenderTarget);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);

    ezRectFloat viewport = ezRectFloat(0, 0, m_pWindow->GetClientAreaSize().width / 2.0f, (float)m_pWindow->GetClientAreaSize().height);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader, ezShaderBindFlags::None);
    ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = ezColor(1, 1, 1, 1);
    ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroupTest.BindBuffer("PerObject", m_hObjectTransformCB);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer(0xFFFFFFFF, 0, 2).IgnoreResult();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  BeginCommands("Texture2DArray");
  {
    TransitionTexture(m_hTexture2DArray, ezGALResourceState::ShaderResource);

    ezRectFloat viewport = ezRectFloat(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

    ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroupTest.BindTexture("DiffuseTexture", m_hTexture2DArray);

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    EndRendering();
    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::Tessellation()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
  BeginCommands("Tessellation");
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
    ezRectFloat viewport = ezRectFloat(0, 0, fWidth, fHeight);
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);
    RenderObject(m_hSphereMesh, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);

    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}


void ezRendererTestAdvancedFeatures::Compute()
{
  BeginCommands("Compute");
  {
    ezUInt32 uiWidth = 8;
    ezUInt32 uiHeight = 8;

    ezRenderContext::GetDefaultInstance()->BeginCompute("Compute");
    {
      ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
      ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();

      ezGALTextureRange textureRange;
      textureRange.m_uiBaseMipLevel = 4;
      textureRange.m_uiMipLevels = 1;
      textureRange.m_uiBaseArraySlice = 0;
      textureRange.m_uiArraySlices = 2;

      TransitionTexture(m_hTexture2D, ezGALResourceState::UnorderedAccess, textureRange);

      bindGroupTest.BindTexture("OutputTexture", m_hTexture2D, textureRange);

      // The compute shader uses [numthreads(8, 8, 1)], so we need to compute how many of these groups we need to dispatch to fill the entire image.
      constexpr ezUInt32 uiThreadsX = 8;
      constexpr ezUInt32 uiThreadsY = 8;
      const ezUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
      const ezUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;
      // As the image is exactly as big as one of our groups, we need to dispatch exactly one group:
      EZ_TEST_INT(uiDispatchX, 1);
      EZ_TEST_INT(uiDispatchY, 1);
      ezRenderContext::GetDefaultInstance()->Dispatch(uiDispatchX, uiDispatchY, 2).AssertSuccess();
    }
    ezRenderContext::GetDefaultInstance()->EndCompute();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;

  const ezMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
  BeginCommands("Texture2D");
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
    TransitionTexture(m_hTexture2D, ezGALResourceState::ShaderResource, m_Texture2DRange);

    m_bCaptureImage = true;
    ezRectFloat viewport = ezRectFloat(0, 0, fWidth, fHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, m_Texture2DRange);
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::MSAAResolve()
{
  // Exercises three currently-untested encoder paths in one pass:
  // 1. Rendering into MSAA color + MSAA depth-stencil targets with stencil writes / tests.
  // 2. ezGALCommandEncoder::Clear inside an active render pass (color-only clear that must preserve the stencil contents).
  // 3. ezGALCommandEncoder::ResolveTexture to downsample the MSAA color into a single-sample texture.
  //
  // Verification is done via readback of the resolved texture so the test does not need a reference image.

  constexpr ezUInt32 uiW = 64;
  constexpr ezUInt32 uiH = 64;

  ezGALDepthStencilStateCreationDescription writeStencilDesc;
  writeStencilDesc.m_bDepthEnable = false;
  writeStencilDesc.m_bDepthWrite = false;
  writeStencilDesc.m_bStencilEnable = true;
  writeStencilDesc.m_uiStencilReadMask = 0xFF;
  writeStencilDesc.m_uiStencilWriteMask = 0xFF;
  writeStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Always;
  writeStencilDesc.m_FrontFaceStencilOp.m_PassOp = ezGALStencilOp::Replace;
  writeStencilDesc.m_BackFaceStencilOp = writeStencilDesc.m_FrontFaceStencilOp;

  ezGALDepthStencilStateCreationDescription testStencilDesc;
  testStencilDesc.m_bDepthEnable = false;
  testStencilDesc.m_bDepthWrite = false;
  testStencilDesc.m_bStencilEnable = true;
  testStencilDesc.m_uiStencilReadMask = 0xFF;
  testStencilDesc.m_uiStencilWriteMask = 0x00;
  testStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Equal;
  testStencilDesc.m_BackFaceStencilOp = testStencilDesc.m_FrontFaceStencilOp;

  ezGALDepthStencilStateHandle hWriteStencil = m_pDevice->CreateDepthStencilState(writeStencilDesc);
  ezGALDepthStencilStateHandle hTestStencil = m_pDevice->CreateDepthStencilState(testStencilDesc);

  // Pre-multiplied identity MVP. The full-screen quad uses a 2x2 NDC rect (geom.AddRect(2x2)), so identity already covers the viewport.
  ezMat4 mFull = ezMat4::MakeIdentity();
  // A centered half-size quad for the stencil-write pass.
  ezMat4 mCenter = ezMat4::MakeScaling(ezVec3(0.5f, 0.5f, 1.0f));
  if (ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped)
  {
    ezMat4 flipY = ezMat4::MakeScaling(ezVec3(1.0f, -1.0f, 1.0f));
    mFull = flipY * mFull;
    mCenter = flipY * mCenter;
  }

  ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();

  BeginCommands("MSAAResolve");
  {
    TransitionTexture(m_hMSAAColor, ezGALResourceState::RenderTarget);
    TransitionTexture(m_hMSAADepthStencil, ezGALResourceState::DepthStencilWrite);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hMSAAColor));
    renderingSetup.SetClearColor(0, ezColor::Black);
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hMSAADepthStencil));
    renderingSetup.SetClearDepth().SetClearStencil();

    ezRectFloat viewport = ezRectFloat(0, 0, (float)uiW, (float)uiH);
    pRenderContext->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    // 1. Write stencil = 1 in the centered quad area. Color writes are not relevant here; we will overwrite color in step 2.
    {
      pRenderContext->SetDepthStencilState(hWriteStencil);
      pRenderContext->SetStencilRefValue(1);

      ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mCenter;
      ocb->m_Color = ezColor::Black;
      ezBindGroupBuilder& bg = pRenderContext->GetBindGroup();
      bg.BindBuffer("PerObject", m_hObjectTransformCB);

      pRenderContext->BindShader(m_hMSAAStencilShader, ezShaderBindFlags::NoDepthStencilState);
      pRenderContext->BindMeshBuffer(m_hMSAAQuadMesh);
      pRenderContext->DrawMeshBuffer().AssertSuccess();
    }

    // 2. Clear color to red but preserve the stencil buffer. This exercises ezGALCommandEncoder::Clear.
    m_pEncoder->Clear(ezColor::Red, 0xFFFFFFFFu, false, false);

    // 3. Draw a full-screen quad in green where stencil == 1. Pixels outside the centered region keep their red color from the Clear.
    {
      pRenderContext->SetDepthStencilState(hTestStencil);
      pRenderContext->SetStencilRefValue(1);

      ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mFull;
      // Use linear pure green directly. ezColor::Green is the HTML #008000 dark green and would readback as ~55/255 due to sRGB->linear conversion.
      ocb->m_Color = ezColor(0.0f, 1.0f, 0.0f);
      ezBindGroupBuilder& bg = pRenderContext->GetBindGroup();
      bg.BindBuffer("PerObject", m_hObjectTransformCB);

      pRenderContext->BindShader(m_hMSAAStencilShader, ezShaderBindFlags::NoDepthStencilState);
      pRenderContext->BindMeshBuffer(m_hMSAAQuadMesh);
      pRenderContext->DrawMeshBuffer().AssertSuccess();
    }

    pRenderContext->EndRendering();

    // 4. Resolve the MSAA color into the single-sample target.
    TransitionTexture(m_hMSAAColor, ezGALResourceState::ResolveSource);
    TransitionTexture(m_hMSAAResolveTarget, ezGALResourceState::ResolveDestination);
    m_pEncoder->ResolveTexture(m_hMSAAResolveTarget, ezGALTextureSubresource(), m_hMSAAColor, ezGALTextureSubresource());

    // 5. Read the resolved texture back so we can verify pixel values.
    TransitionTexture(m_hMSAAResolveTarget, ezGALResourceState::CopySource);
    m_MSAAReadback.ReadbackTexture(*m_pEncoder, m_hMSAAResolveTarget);
  }
  EndCommands();

  m_pDevice->DestroyDepthStencilState(hWriteStencil);
  m_pDevice->DestroyDepthStencilState(hTestStencil);

  ezEnum<ezGALAsyncResult> res = m_MSAAReadback.GetReadbackResult(ezTime::MakeFromHours(1));
  if (!EZ_TEST_BOOL_MSG(res == ezGALAsyncResult::Ready, "MSAA readback timed out"))
    return;

  ezGALTextureSubresource sub;
  ezArrayPtr<ezGALTextureSubresource> subs(&sub, 1);
  ezTempHybridArray<ezGALSystemMemoryDescription, 1> memory;
  ezReadbackTextureLock lock = m_MSAAReadback.LockTexture(subs, memory);
  EZ_ASSERT_ALWAYS(lock, "Failed to lock MSAA readback texture");

  // BGRAUByteNormalized, 4 bytes per pixel as B, G, R, A.
  auto sampleBGRA = [&](ezUInt32 x, ezUInt32 y) -> ezColorLinearUB
  {
    const ezUInt8* pRow = static_cast<const ezUInt8*>(memory[0].m_pData.GetPtr()) + memory[0].m_uiRowPitch * y;
    const ezUInt8* p = pRow + x * 4;
    return ezColorLinearUB(p[2], p[1], p[0], p[3]);
  };

  // The center is inside the stencil-marked region, so it must be green.
  const ezColorLinearUB centerPixel = sampleBGRA(uiW / 2, uiH / 2);
  EZ_TEST_INT(centerPixel.r, 0);
  EZ_TEST_BOOL_MSG(centerPixel.g > 200, "Center pixel should be green (stencil pass)");
  EZ_TEST_INT(centerPixel.b, 0);

  // The corner is outside the stencil-marked region, so the encoder Clear color (red) must be visible.
  const ezColorLinearUB cornerPixel = sampleBGRA(1, 1);
  EZ_TEST_BOOL_MSG(cornerPixel.r > 200, "Corner pixel should be red (encoder Clear, stencil != 1)");
  EZ_TEST_INT(cornerPixel.g, 0);
  EZ_TEST_INT(cornerPixel.b, 0);
}

void ezRendererTestAdvancedFeatures::DepthBias()
{
  // Verifies ezGALRasterizerStateCreationDescription::m_iDepthBias, m_fSlopeScaledDepthBias and m_fDepthBiasClamp.
  //
  // Every case gets its own square cell of the render target. In each cell a reference quad is drawn with an unbiased rasterizer state and depth writes enabled, then the exact same quad is drawn again, moved away from the viewer by a fixed gap and rasterized with the depth bias under test. Because both quads are coplanar apart from that gap, the biased quad passes the 'Less' depth test if and only if the applied bias exceeds the gap towards the viewer. The result is therefore a binary green (biased quad won) / red (reference quad still visible) per cell, which is
  // verified via readback instead of a reference image.
  //
  // All bias magnitudes are expressed in multiples of the depth format's smallest resolvable difference, which is what m_iDepthBias is counted in. The exact value is implementation defined within a factor of two, hence the generous margins between the gap and the expected biases.

  const bool bSupportsClamp = m_pDevice->GetCapabilities().m_bSupportsDepthBiasClamp;
  if (!bSupportsClamp)
  {
    ezLog::Info("The depth bias clamp is not supported by this device, skipping the cases that rely on it.");
  }

  ezGALDepthStencilStateCreationDescription writeDepthDesc;
  writeDepthDesc.m_bDepthEnable = true;
  writeDepthDesc.m_bDepthWrite = true;
  writeDepthDesc.m_DepthTestFunc = ezGALCompareFunc::Always;

  ezGALDepthStencilStateCreationDescription testDepthDesc;
  testDepthDesc.m_bDepthEnable = true;
  testDepthDesc.m_bDepthWrite = false;
  testDepthDesc.m_DepthTestFunc = ezGALCompareFunc::Less;

  ezGALDepthStencilStateHandle hWriteDepth = m_pDevice->CreateDepthStencilState(writeDepthDesc);
  ezGALDepthStencilStateHandle hTestDepth = m_pDevice->CreateDepthStencilState(testDepthDesc);

  ezGALRasterizerStateCreationDescription rasterDesc;
  rasterDesc.m_CullMode = ezGALCullMode::None;
  ezGALRasterizerStateHandle hNoBias = m_pDevice->CreateRasterizerState(rasterDesc);

  ezHybridArray<ezGALRasterizerStateHandle, s_uiDepthBiasCaseCount> biasStates;
  for (const DepthBiasCase& testCase : s_DepthBiasCases)
  {
    ezGALRasterizerStateCreationDescription desc;
    desc.m_CullMode = ezGALCullMode::None;
    desc.m_iDepthBias = testCase.m_iDepthBias;
    desc.m_fSlopeScaledDepthBias = testCase.m_fSlopeScaledDepthBias;
    desc.m_fDepthBiasClamp = testCase.m_fClampUnits * m_fDepthBiasUnit;
    biasStates.PushBack(m_pDevice->CreateRasterizerState(desc));
  }

  // Maps the quad's local NDC space onto the cell of the given case and places it at s_fDepthBiasBaseDepth, optionally tilted along x.
  auto MakeTransform = [](ezUInt32 uiCase, bool bSloped, float fDepthOffset) -> ezMat4
  {
    const float fScaleX = 1.0f / s_uiDepthBiasCaseCount;
    const float fCenterX = -1.0f + (2.0f * uiCase + 1.0f) * fScaleX;

    ezMat4 m = ezMat4::MakeIdentity();
    m.SetRow(0, ezVec4(fScaleX, 0.0f, 0.0f, fCenterX));
    m.SetRow(2, ezVec4(bSloped ? s_fDepthBiasSlope : 0.0f, 0.0f, 1.0f, s_fDepthBiasBaseDepth + fDepthOffset));
    return m;
  };

  ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();

  auto DrawQuad = [&](const ezMat4& mTransform, const ezColor& color, ezGALDepthStencilStateHandle hDepthStencil, ezGALRasterizerStateHandle hRasterizer)
  {
    pRenderContext->SetDepthStencilState(hDepthStencil);
    pRenderContext->SetRasterizerState(hRasterizer);

    ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mTransform;
    ocb->m_Color = color;
    pRenderContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);

    pRenderContext->BindShader(m_hDepthBiasShader, ezShaderBindFlags::NoRasterizerState | ezShaderBindFlags::NoDepthStencilState);
    pRenderContext->BindMeshBuffer(m_hDepthBiasQuadMesh);
    pRenderContext->DrawMeshBuffer().AssertSuccess();
  };

  BeginCommands("DepthBias");
  {
    TransitionTexture(m_hDepthBiasColor, ezGALResourceState::RenderTarget);
    TransitionTexture(m_hDepthBiasDepth, ezGALResourceState::DepthStencilWrite);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hDepthBiasColor));
    renderingSetup.SetClearColor(0, ezColor::Black);
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthBiasDepth));
    renderingSetup.SetClearDepth();

    ezRectFloat viewport = ezRectFloat(0, 0, (float)(s_uiDepthBiasCellSize * s_uiDepthBiasCaseCount), (float)s_uiDepthBiasCellSize);
    pRenderContext->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    for (ezUInt32 i = 0; i < s_uiDepthBiasCaseCount; ++i)
    {
      const DepthBiasCase& testCase = s_DepthBiasCases[i];
      DrawQuad(MakeTransform(i, testCase.m_bSloped, 0.0f), ezColor(1.0f, 0.0f, 0.0f), hWriteDepth, hNoBias);
      DrawQuad(MakeTransform(i, testCase.m_bSloped, s_fDepthBiasGapUnits * m_fDepthBiasUnit), ezColor(0.0f, 1.0f, 0.0f), hTestDepth, biasStates[i]);
    }

    pRenderContext->EndRendering();

    TransitionTexture(m_hDepthBiasColor, ezGALResourceState::CopySource);
    m_DepthBiasReadback.ReadbackTexture(*m_pEncoder, m_hDepthBiasColor);
  }
  EndCommands();

  m_pDevice->DestroyDepthStencilState(hWriteDepth);
  m_pDevice->DestroyDepthStencilState(hTestDepth);
  m_pDevice->DestroyRasterizerState(hNoBias);
  for (ezGALRasterizerStateHandle hState : biasStates)
  {
    m_pDevice->DestroyRasterizerState(hState);
  }

  ezEnum<ezGALAsyncResult> res = m_DepthBiasReadback.GetReadbackResult(ezTime::MakeFromHours(1));
  if (!EZ_TEST_BOOL_MSG(res == ezGALAsyncResult::Ready, "Depth bias readback timed out"))
    return;

  ezGALTextureSubresource sub;
  ezArrayPtr<ezGALTextureSubresource> subs(&sub, 1);
  ezTempHybridArray<ezGALSystemMemoryDescription, 1> memory;
  ezReadbackTextureLock lock = m_DepthBiasReadback.LockTexture(subs, memory);
  EZ_ASSERT_ALWAYS(lock, "Failed to lock depth bias readback texture");

  // BGRAUByteNormalized, 4 bytes per pixel as B, G, R, A.
  auto SampleBGRA = [&](ezUInt32 x, ezUInt32 y) -> ezColorLinearUB
  {
    const ezUInt8* pRow = static_cast<const ezUInt8*>(memory[0].m_pData.GetPtr()) + memory[0].m_uiRowPitch * y;
    const ezUInt8* p = pRow + x * 4;
    return ezColorLinearUB(p[2], p[1], p[0], p[3]);
  };

  for (ezUInt32 i = 0; i < s_uiDepthBiasCaseCount; ++i)
  {
    const DepthBiasCase& testCase = s_DepthBiasCases[i];
    if (testCase.m_fClampUnits != 0.0f && !bSupportsClamp)
      continue;

    const ezColorLinearUB pixel = SampleBGRA(i * s_uiDepthBiasCellSize + s_uiDepthBiasCellSize / 2, s_uiDepthBiasCellSize / 2);
    const bool bVisible = pixel.g > 200 && pixel.r < 55;
    const bool bHidden = pixel.r > 200 && pixel.g < 55;

    if (!EZ_TEST_BOOL_MSG(bVisible || bHidden, "'%s': neither quad is clearly visible, got RGB (%d, %d, %d)", testCase.m_szName, (int)pixel.r, (int)pixel.g, (int)pixel.b))
      continue;

    EZ_TEST_BOOL_MSG(bVisible == testCase.m_bExpectVisible, "'%s': the biased quad is %s but was expected to be %s", testCase.m_szName, bVisible ? "visible" : "hidden", testCase.m_bExpectVisible ? "visible" : "hidden");
  }
}

void ezRendererTestAdvancedFeatures::ConservativeRasterization()
{
  // Verifies ezGALRasterizerStateCreationDescription::m_bConservativeRasterization.
  //
  // Every case gets its own square cell of the render target and draws a single white quad into it. Standard rasterization only produces a fragment when the pixel center lies inside the primitive, conservative (overestimated) rasterization produces one for every pixel the primitive touches at all. Feeding a quad that is smaller than a pixel and placed so that it misses that pixel's center therefore yields nothing without the feature and at least one lit pixel with it. The two remaining cells draw a quad that covers whole pixels to confirm that ordinary geometry is unaffected.
  //
  // The number of lit pixels per cell is verified via readback instead of a reference image.

  constexpr ezUInt32 uiWidth = s_uiConservativeRasterCellSize * s_uiConservativeRasterCaseCount;
  constexpr ezUInt32 uiHeight = s_uiConservativeRasterCellSize;

  // Maps the quad's local NDC space onto the given window space rect of the render target.
  auto MakeTransform = [](float fMinX, float fMinY, float fMaxX, float fMaxY) -> ezMat4
  {
    ezMat4 m = ezMat4::MakeIdentity();
    m.SetRow(0, ezVec4((fMaxX - fMinX) / uiWidth, 0.0f, 0.0f, (fMinX + fMaxX) / uiWidth - 1.0f));
    m.SetRow(1, ezVec4(0.0f, (fMaxY - fMinY) / uiHeight, 0.0f, (fMinY + fMaxY) / uiHeight - 1.0f));
    return m;
  };

  ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();

  ezHybridArray<ezGALRasterizerStateHandle, s_uiConservativeRasterCaseCount> rasterStates;
  for (const ConservativeRasterCase& testCase : s_ConservativeRasterCases)
  {
    ezGALRasterizerStateCreationDescription desc;
    desc.m_CullMode = ezGALCullMode::None;
    desc.m_bConservativeRasterization = testCase.m_bConservative;
    ezGALRasterizerStateHandle hState = m_pDevice->CreateRasterizerState(desc);
    if (!EZ_TEST_BOOL_MSG(!hState.IsInvalidated(), "'%s': failed to create the rasterizer state", testCase.m_szName))
      return;

    rasterStates.PushBack(hState);
  }

  BeginCommands("ConservativeRasterization");
  {
    TransitionTexture(m_hConservativeRasterColor, ezGALResourceState::RenderTarget);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hConservativeRasterColor));
    renderingSetup.SetClearColor(0, ezColor::Black);

    ezRectFloat viewport = ezRectFloat(0, 0, (float)uiWidth, (float)uiHeight);
    pRenderContext->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    for (ezUInt32 i = 0; i < s_uiConservativeRasterCaseCount; ++i)
    {
      const ConservativeRasterCase& testCase = s_ConservativeRasterCases[i];
      const float fCellOffset = (float)(i * s_uiConservativeRasterCellSize);
      const float fMin = testCase.m_bSubPixel ? s_fConservativeRasterSubPixelMin : s_fConservativeRasterCoveringMin;
      const float fMax = testCase.m_bSubPixel ? s_fConservativeRasterSubPixelMax : s_fConservativeRasterCoveringMax;

      pRenderContext->SetRasterizerState(rasterStates[i]);

      ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = MakeTransform(fCellOffset + fMin, fMin, fCellOffset + fMax, fMax);
      ocb->m_Color = ezColor::White;
      pRenderContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);

      pRenderContext->BindShader(m_hConservativeRasterShader, ezShaderBindFlags::NoRasterizerState);
      pRenderContext->BindMeshBuffer(m_hConservativeRasterQuadMesh);
      pRenderContext->DrawMeshBuffer().AssertSuccess();
    }

    pRenderContext->EndRendering();
    TransitionTexture(m_hConservativeRasterColor, ezGALResourceState::CopySource);
    m_ConservativeRasterReadback.ReadbackTexture(*m_pEncoder, m_hConservativeRasterColor);
  }
  EndCommands();

  for (ezGALRasterizerStateHandle hState : rasterStates)
  {
    m_pDevice->DestroyRasterizerState(hState);
  }

  ezEnum<ezGALAsyncResult> res = m_ConservativeRasterReadback.GetReadbackResult(ezTime::MakeFromHours(1));
  if (!EZ_TEST_BOOL_MSG(res == ezGALAsyncResult::Ready, "Conservative rasterization readback timed out"))
    return;

  ezGALTextureSubresource sub;
  ezArrayPtr<ezGALTextureSubresource> subs(&sub, 1);
  ezTempHybridArray<ezGALSystemMemoryDescription, 1> memory;
  ezReadbackTextureLock lock = m_ConservativeRasterReadback.LockTexture(subs, memory);
  EZ_ASSERT_ALWAYS(lock, "Failed to lock conservative rasterization readback texture");

  for (ezUInt32 i = 0; i < s_uiConservativeRasterCaseCount; ++i)
  {
    const ConservativeRasterCase& testCase = s_ConservativeRasterCases[i];

    ezUInt32 uiLitPixels = 0;
    for (ezUInt32 y = 0; y < s_uiConservativeRasterCellSize; ++y)
    {
      // BGRAUByteNormalized, 4 bytes per pixel as B, G, R, A.
      const ezUInt8* pRow = static_cast<const ezUInt8*>(memory[0].m_pData.GetPtr()) + memory[0].m_uiRowPitch * y;
      for (ezUInt32 x = 0; x < s_uiConservativeRasterCellSize; ++x)
      {
        if (pRow[(i * s_uiConservativeRasterCellSize + x) * 4] > 128)
          ++uiLitPixels;
      }
    }

    EZ_TEST_BOOL_MSG(uiLitPixels >= testCase.m_uiMinLitPixels && uiLitPixels <= testCase.m_uiMaxLitPixels, "'%s': %d pixels are lit, expected between %d and %d", testCase.m_szName, (int)uiLitPixels, (int)testCase.m_uiMinLitPixels, (int)testCase.m_uiMaxLitPixels);
  }
}

ezTestAppRun ezRendererTestAdvancedFeatures::Material()
{
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::BlockTillLoaded);
    const ezMaterialResourceDescriptor& desc = pMaterial->GetCurrentDesc();
    EZ_TEST_INT(desc.m_PermutationVars.GetCount(), 0);
    EZ_TEST_INT(desc.m_Parameters.GetCount(), 2);
    EZ_TEST_INT(desc.m_Texture2DBindings.GetCount(), 1);
    EZ_TEST_INT(desc.m_TextureCubeBindings.GetCount(), 0);
    ezVariant color1 = pMaterial->GetParameter(m_sBaseColor);
    ezVariant color2 = pMaterial->GetParameter(m_sBaseColor2);
    ezTexture2DResourceHandle hTexture = pMaterial->GetTexture2DBinding(m_sTexture);

    if (m_iFrame == ImageCaptureFrames::Material_ColorChange)
    {
      EZ_TEST_BOOL(color1.IsA<ezColor>() && color1.Get<ezColor>() == ezColor::White);
      EZ_TEST_BOOL(color2.IsA<ezColor>() && color2.Get<ezColor>() == ezColor::White);
      EZ_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetParameter(m_sBaseColor, ezColor::Yellow);
    }
    else if (m_iFrame == ImageCaptureFrames::Material_ColorChange2)
    {
      EZ_TEST_BOOL(color1.IsA<ezColor>() && color1.Get<ezColor>() == ezColor::Yellow);
      EZ_TEST_BOOL(color2.IsA<ezColor>() && color2.Get<ezColor>() == ezColor::White);
      EZ_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetParameter(m_sBaseColor2, ezColor::Cyan);
    }
    else if (m_iFrame == ImageCaptureFrames::Material_ChangeTexture)
    {
      EZ_TEST_BOOL(color1.IsA<ezColor>() && color1.Get<ezColor>() == ezColor::Yellow);
      EZ_TEST_BOOL(color2.IsA<ezColor>() && color2.Get<ezColor>() == ezColor::Cyan);
      EZ_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetTexture2DBinding(m_sTexture, m_hTexture2);
    }
  }

  BeginFrame();
  {
    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    const ezMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
    BeginCommands("MaterialTest");
    {
      TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
      ezRectFloat viewport = ezRectFloat(0, 0, fWidth, fHeight);
      ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

      ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
      pContext->SetAllowAsyncShaderLoading(false);
      pContext->BindMaterial(m_hMaterial);

      ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mMVP;
      ocb->m_Color = ezColor(1, 1, 1, 1);

      ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroupTest.BindBuffer("PerObject", m_hObjectTransformCB);

      ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
      ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      EndRendering();
      if (m_ImgCompFrames.Contains(m_iFrame))
      {
        TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
        EZ_TEST_IMAGE(m_iFrame, 100);
      }
    }
    EndCommands();
  }
  EndFrame();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return ezTestAppRun::Quit;
  }
  return ezTestAppRun::Continue;
}

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
ezTestAppRun ezRendererTestAdvancedFeatures::SharedTexture()
{
  if (m_pOffscreenProcess->GetState() != ezProcessState::Running)
  {
    EZ_TEST_BOOL(m_bExiting);
    return ezTestAppRun::Quit;
  }

  m_pProtocol->WaitForMessages(ezTime::MakeFromMilliseconds(16)).IgnoreResult();

  ezOffscreenTest_SharedTexture texture = m_SharedTextureQueue.PeekFront();
  m_SharedTextureQueue.PopFront();

  ezStringBuilder sTemp;
  sTemp.SetFormat("Render {}:{}|{}", m_uiReceivedTextures, texture.m_uiCurrentTextureIndex, texture.m_uiCurrentSemaphoreValue);
  EZ_PROFILE_SCOPE(sTemp);
  BeginFrame();
  {
    const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(m_hSharedTextures[texture.m_uiCurrentTextureIndex]);
    EZ_ASSERT_DEV(pSharedTexture != nullptr, "Shared texture did not resolve");

    pSharedTexture->WaitSemaphoreGPU(texture.m_uiCurrentSemaphoreValue);

    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    const ezUInt32 uiColumns = 1;
    const ezUInt32 uiRows = 1;
    const float fElementWidth = fWidth / uiColumns;
    const float fElementHeight = fHeight / uiRows;

    const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
    BeginCommands("Texture2D");
    {
      TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);
      TransitionTexture(m_hSharedTextures[texture.m_uiCurrentTextureIndex], ezGALResourceState::ShaderResource);

      ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
      m_bCaptureImage = true;
      viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);

      ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

      ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroupTest.BindTexture("DiffuseTexture", m_hSharedTextures[texture.m_uiCurrentTextureIndex]);
      RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);

      EndRendering();
      if (!m_bExiting && m_uiReceivedTextures > 10)
      {
        TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
        EZ_TEST_IMAGE(0, 10);

        ezOffscreenTest_CloseMsg msg;
        EZ_TEST_BOOL(m_pProtocol->Send(&msg));
        m_bExiting = true;
      }
    }
    EndCommands();

    texture.m_uiCurrentSemaphoreValue++;
    pSharedTexture->SignalSemaphoreGPU(texture.m_uiCurrentSemaphoreValue);
  }
  EndFrame();

  if (m_SharedTextureQueue.IsEmpty() || !m_pChannel->IsConnected())
  {
    m_SharedTextureQueue.PushBack(texture);
  }
  else if (!m_bExiting)
  {
    ezOffscreenTest_RenderMsg msg;
    msg.m_Texture = texture;
    EZ_TEST_BOOL(m_pProtocol->Send(&msg));
  }

  return ezTestAppRun::Continue;
}

void ezRendererTestAdvancedFeatures::OffscreenProcessMessageFunc(const ezIpcProcessMessageProtocol::Event& msg)
{
  if (const auto* pAction = ezDynamicCast<const ezOffscreenTest_RenderResponseMsg*>(msg.m_pMessage))
  {
    m_uiReceivedTextures++;
    ezStringBuilder sTemp;
    sTemp.SetFormat("Receive {}|{}", pAction->m_Texture.m_uiCurrentTextureIndex, pAction->m_Texture.m_uiCurrentSemaphoreValue);
    EZ_PROFILE_SCOPE(sTemp);
    m_SharedTextureQueue.PushBack(pAction->m_Texture);
  }
}
#endif

static ezRendererTestAdvancedFeatures g_AdvancedFeaturesTest;
