#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphUtils.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

ezRenderGraphTextureHandle ezRenderGraphUtils::GenerateMipMaps(ezGALTextureHandle hTexture, ezGALTextureRange range, ezRenderGraph& ref_renderGraph)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  EZ_ASSERT_DEV(pDevice != nullptr, "No GAL device available.");

  const ezGALTexture* pTexture = pDevice->GetTexture(hTexture);
  EZ_ASSERT_DEV(pTexture != nullptr, "GenerateMipMaps called with invalid texture handle.");
  if (pTexture == nullptr)
    return {};

  const ezGALTextureCreationDescription& desc = pTexture->GetDescription();
  EZ_ASSERT_DEV(desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget), "RenderTarget usage required to create mip maps");
  EZ_ASSERT_DEV(desc.m_SampleCount == ezGALMSAASampleCount::None, "Generating mipmaps for MSAA textures is not supported.");
  EZ_ASSERT_DEV(desc.m_uiMipLevelCount > 1, "Texture has no mip chain to generate.");
  EZ_ASSERT_DEV(!ezGALResourceFormat::IsDepthFormat(desc.m_Format), "Generating mipmaps for depth textures is not supported.");
  EZ_ASSERT_DEV(desc.m_Type == ezGALTextureType::Texture2D || desc.m_Type == ezGALTextureType::Texture2DArray || desc.m_Type == ezGALTextureType::TextureCube,
    "GenerateMipMaps currently supports Texture2D, Texture2DArray and TextureCube only.");

  range = pTexture->ClampRange(range);

  ezRenderGraphTextureHandle hGraphTexture = ref_renderGraph.ImportTexture(hTexture);

  if (range.m_uiMipLevels <= 1)
    return hGraphTexture;

  ezShaderResourceHandle hDownscaleShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Downscale.ezShader");
  EZ_ASSERT_DEV(hDownscaleShader.IsValid(), "Could not load Downscale shader.");

  static ezHashedString sCameraMode = ezMakeHashedString("CAMERA_MODE");
  static ezHashedString sPerspective = ezMakeHashedString("CAMERA_MODE_PERSPECTIVE");

  const ezUInt32 uiArraySliceEnd = range.m_uiBaseArraySlice + range.m_uiArraySlices;
  const ezUInt32 uiMipEnd = range.m_uiBaseMipLevel + range.m_uiMipLevels;

  ref_renderGraph.PushMarker("GenerateMipMaps");
  for (ezUInt32 uiArraySlice = range.m_uiBaseArraySlice; uiArraySlice < uiArraySliceEnd; ++uiArraySlice)
  {
    for (ezUInt32 uiMipLevel = range.m_uiBaseMipLevel; uiMipLevel < uiMipEnd - 1; ++uiMipLevel)
    {
      const ezUInt8 uiSourceMip = static_cast<ezUInt8>(uiMipLevel);
      const ezUInt8 uiTargetMip = static_cast<ezUInt8>(uiMipLevel + 1);
      const ezUInt16 uiSlice = static_cast<ezUInt16>(uiArraySlice);

      ezGALTextureRange sourceRange{uiSlice, 1, uiSourceMip, 1};
      ezGALRenderTargetRange targetRange{uiSlice, 1, uiTargetMip};

      auto pass = ref_renderGraph.AddGraphicsPass("GenerateMipMaps");
      pass.ReadTexture(hGraphTexture, sourceRange, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
      pass.AddColorTarget(hGraphTexture, targetRange, {}, {}, ezGALResourceFormat::Invalid, ezGALTextureType::Texture2DArray);
      pass.HasSideEffects();
      pass.SetExecuteCallback([hGraphTexture, sourceRange, hDownscaleShader](const ezRenderGraphContext& context)
        {
          ezRenderContext* pRenderContext = context.GetRenderContext();
          pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);
          pRenderContext->BindShader(hDownscaleShader);

          ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
          bindGroup.BindTexture("Input", context.ResolveTexture(hGraphTexture), sourceRange, ezGALResourceFormat::Invalid, ezGALTextureType::Texture2DArray);

          pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
          pRenderContext->DrawMeshBuffer().IgnoreResult(); });
    }
  }
  ref_renderGraph.PopMarker();
  return hGraphTexture;
}
