#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraphUtils.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionIrradianceConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReflectionFilterPass, 2, ezRTTIDefaultAllocator<ezReflectionFilterPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FilteredSpecular", m_PinFilteredSpecular),
    EZ_MEMBER_PROPERTY("AvgLuminance", m_PinAvgLuminance),
    EZ_MEMBER_PROPERTY("IrradianceData", m_PinIrradianceData),
    EZ_MEMBER_PROPERTY("DiffuseIntensity", m_fDiffuseIntensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("DiffuseSaturation", m_fDiffuseSaturation)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SpecularIntensity", m_fSpecularIntensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SpecularOutputIndex", m_uiSpecularOutputIndex),
    EZ_MEMBER_PROPERTY("IrradianceOutputIndex", m_uiIrradianceOutputIndex),
    EZ_ACCESSOR_PROPERTY("InputCubemap", GetInputCubemap, SetInputCubemap)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezReflectionFilterPass::ezReflectionFilterPass()
  : ezRenderPipelinePass("ReflectionFilterPass")

{
  {
    m_hFilteredSpecularConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezReflectionFilteredSpecularConstants>();
    m_hFilteredSpecularShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/ReflectionFilteredSpecular.ezShader");
    EZ_ASSERT_DEV(m_hFilteredSpecularShader.IsValid(), "Could not load ReflectionFilteredSpecular shader!");

    m_hIrradianceConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezReflectionIrradianceConstants>();
    m_hIrradianceShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/ReflectionIrradiance.ezShader");
    EZ_ASSERT_DEV(m_hIrradianceShader.IsValid(), "Could not load ReflectionIrradiance shader!");
  }
}

ezReflectionFilterPass::~ezReflectionFilterPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hIrradianceConstantBuffer);
}

ezStatus ezReflectionFilterPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  // Create filtered specular output
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = ezReflectionPool::GetReflectionCubeMapSize();
    desc.m_uiHeight = desc.m_uiWidth;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_Type = ezGALTextureType::TextureCube;
    desc.m_TextureFlags.Add(ezGALTextureUsageFlags::UnorderedAccess);
    desc.m_uiMipLevelCount = ezMath::Log2i(desc.m_uiWidth) - 1;
    ezRenderGraphTextureHandle hFilteredSpecular = ref_graph.CreateTexture(desc);
    outputs[m_PinFilteredSpecular.m_uiOutputIndex].m_TextureHandle = hFilteredSpecular;
  }

  // Create average luminance output (todo, unused)
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 4;
    desc.m_uiHeight = 4;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_TextureFlags.Add(ezGALTextureUsageFlags::RenderTarget | ezGALTextureUsageFlags::UnorderedAccess);
    desc.m_ResourceAccess.m_bImmutable = false;
    ezRenderGraphTextureHandle hAvgLuminance = ref_graph.CreateTexture(desc);
    outputs[m_PinAvgLuminance.m_uiOutputIndex].m_TextureHandle = hAvgLuminance;
  }

  // Create irradiance output
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 6;
    desc.m_uiHeight = 64;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_TextureFlags.Add(ezGALTextureUsageFlags::RenderTarget | ezGALTextureUsageFlags::UnorderedAccess);
    desc.m_ResourceAccess.m_bImmutable = false;
    ezRenderGraphTextureHandle hIrradianceData = ref_graph.CreateTexture(desc);
    outputs[m_PinIrradianceData.m_uiOutputIndex].m_TextureHandle = hIrradianceData;
  }

  ezRenderGraphTextureHandle hFilteredSpecular = outputs[m_PinFilteredSpecular.m_uiOutputIndex].m_TextureHandle;
  ezRenderGraphTextureHandle hIrradianceData = outputs[m_PinIrradianceData.m_uiOutputIndex].m_TextureHandle;

  // Generate mipmaps
  ezRenderGraphTextureHandle hInputCubeTexture;
  auto pInputCubemap = ref_graph.GetDevice()->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
    return EZ_SUCCESS;
  if (pInputCubemap->GetDescription().m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget))
  {
    hInputCubeTexture = ezRenderGraphUtils::GenerateMipMaps(m_hInputCubemap, {}, ref_graph);
  }
  else
  {
    hInputCubeTexture = ref_graph.ImportTexture(m_hInputCubemap);
  }

  // Filtered specular compute pass
  {
    auto pass = ref_graph.AddComputePass("ReflectionFilterSpecular");
    pass.ReadTexture(hInputCubeTexture, {}, ezGALResourceState::ShaderResource);
    pass.WriteTexture(hFilteredSpecular, {}, ezGALResourceState::UnorderedAccess);
    pass.HasSideEffects();
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
        const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
        ezGALDevice* pDevice = ctx.GetDevice();

        // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
        ezResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
        ezResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
        bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
        renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

        EZ_SCOPE_EXIT(
          renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

        ezGALTextureHandle hResolvedSpecular = ctx.ResolveTexture(hFilteredSpecular);
        const ezGALTexture* pSpecularTexture = pDevice->GetTexture(hResolvedSpecular);
        if (pSpecularTexture == nullptr)
          return;

        const auto& specDesc = pSpecularTexture->GetDescription();
        ezUInt32 uiNumMipMaps = specDesc.m_uiMipLevelCount;
        ezUInt32 uiWidth = specDesc.m_uiWidth;
        ezUInt32 uiHeight = specDesc.m_uiHeight;

        ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindTexture("InputCubemap", ctx.ResolveTexture(hInputCubeTexture));
        bindGroup.BindBuffer("ezReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

        for (ezUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
        {
          ezGALTextureRange textureRange;
          textureRange.m_uiBaseMipLevel = uiMipMapIndex;
          textureRange.m_uiBaseArraySlice = m_uiSpecularOutputIndex * 6;
          textureRange.m_uiArraySlices = 6;
          bindGroup.BindTexture("ReflectionOutput", hResolvedSpecular, textureRange);
          UpdateFilteredSpecularConstantBuffer(uiMipMapIndex, uiNumMipMaps, uiWidth, uiHeight);

          constexpr ezUInt32 uiThreadsX = 8;
          constexpr ezUInt32 uiThreadsY = 8;
          const ezUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
          const ezUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;

          renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();

          uiWidth >>= 1;
          uiHeight >>= 1;
        } //
      });
  }

  {
    // Irradiance
    auto pass = ref_graph.AddComputePass("Irradiance");
    pass.ReadTexture(hInputCubeTexture, {}, ezGALResourceState::ShaderResource);
    pass.WriteTexture(hIrradianceData, {}, ezGALResourceState::UnorderedAccess);
    pass.HasSideEffects();
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
        const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

        UpdateIrradianceConstantBuffer();
        ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindTexture("IrradianceOutput", ctx.ResolveTexture(hIrradianceData));
        bindGroup.BindTexture("InputCubemap", ctx.ResolveTexture(hInputCubeTexture));
        bindGroup.BindBuffer("ezReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
        renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

        renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult(); //
      });
  }

  return EZ_SUCCESS;
}

ezResult ezReflectionFilterPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fDiffuseIntensity;
  inout_stream << m_fDiffuseSaturation;
  inout_stream << m_fSpecularIntensity;
  inout_stream << m_uiSpecularOutputIndex;
  inout_stream << m_uiIrradianceOutputIndex;
  // inout_stream << m_hInputCubemap; Runtime only property
  return EZ_SUCCESS;
}

ezResult ezReflectionFilterPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  inout_stream >> m_fDiffuseIntensity;
  inout_stream >> m_fDiffuseSaturation;
  if (uiVersion >= 2)
  {
    inout_stream >> m_fSpecularIntensity;
  }
  inout_stream >> m_uiSpecularOutputIndex;
  inout_stream >> m_uiIrradianceOutputIndex;
  return EZ_SUCCESS;
}

ezUInt32 ezReflectionFilterPass::GetInputCubemap() const
{
  return m_hInputCubemap.GetInternalID().m_Data;
}

void ezReflectionFilterPass::SetInputCubemap(ezUInt32 uiCubemapHandle)
{
  ezGALTextureHandle hNewCubemapHandle = ezGALTextureHandle(ezGAL::ez18_14Id(uiCubemapHandle));
  if (m_hInputCubemap != hNewCubemapHandle)
  {
    m_hInputCubemap = ezGALTextureHandle(ezGAL::ez18_14Id(uiCubemapHandle));
  }
}

void ezReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(ezUInt32 uiMipMapIndex, ezUInt32 uiNumMipMaps, ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  auto constants = ezRenderContext::GetConstantBufferData<ezReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->MipLevel = uiMipMapIndex;
  constants->OutputWidth = uiWidth;
  constants->OutputHeight = uiHeight;
  constants->Intensity = m_fSpecularIntensity;
}

void ezReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
  auto constants = ezRenderContext::GetConstantBufferData<ezReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity = m_fDiffuseIntensity;
  constants->Saturation = m_fDiffuseSaturation;
  constants->OutputIndex = m_uiIrradianceOutputIndex;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
