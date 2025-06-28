#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionIrradianceConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReflectionFilterPass, 1, ezRTTIDefaultAllocator<ezReflectionFilterPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FilteredSpecular", m_PinFilteredSpecular),
    EZ_MEMBER_PROPERTY("AvgLuminance", m_PinAvgLuminance),
    EZ_MEMBER_PROPERTY("IrradianceData", m_PinIrradianceData),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
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
  m_hIrradianceConstantBuffer.Invalidate();
}

bool ezReflectionFilterPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = ezReflectionPool::GetReflectionCubeMapSize();
    desc.m_uiHeight = desc.m_uiWidth;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_Type = ezGALTextureType::TextureCube;
    desc.m_bAllowUAV = true;
    desc.m_uiMipLevelCount = ezMath::Log2i(desc.m_uiWidth) - 1;
    outputs[m_PinFilteredSpecular.m_uiOutputIndex] = desc;
  }

  return true;
}

void ezReflectionFilterPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto pInputCubemap = pDevice->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
  {
    return;
  }

  // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
  ezResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
  ezResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
  bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  EZ_SCOPE_EXIT(
    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  if (pInputCubemap->GetDescription().m_bAllowDynamicMipGeneration)
  {
    renderViewContext.m_pRenderContext->GetCommandEncoder()->GenerateMipMaps(m_hInputCubemap, {});
  }

  {
    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated() && pDevice->GetTexture(pFilteredSpecularOutput->m_TextureHandle) != nullptr)
    {
      ezUInt32 uiNumMipMaps = pFilteredSpecularOutput->m_Desc.m_uiMipLevelCount;

      ezUInt32 uiWidth = pFilteredSpecularOutput->m_Desc.m_uiWidth;
      ezUInt32 uiHeight = pFilteredSpecularOutput->m_Desc.m_uiHeight;

      auto computeScope = ezRenderContext::BeginComputeScope(renderViewContext, "ReflectionFilter");
      ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindTexture("InputCubemap", m_hInputCubemap);
      bindGroup.BindBuffer("ezReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

      for (ezUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        ezGALTextureRange textureRange;
        textureRange.m_uiBaseMipLevel = uiMipMapIndex;
        textureRange.m_uiBaseArraySlice = m_uiSpecularOutputIndex * 6;
        textureRange.m_uiArraySlices = 6;
        bindGroup.BindTexture("ReflectionOutput", pFilteredSpecularOutput->m_TextureHandle, textureRange);
        UpdateFilteredSpecularConstantBuffer(uiMipMapIndex, uiNumMipMaps, uiWidth, uiHeight);

        constexpr ezUInt32 uiThreadsX = 8;
        constexpr ezUInt32 uiThreadsY = 8;
        const ezUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
        const ezUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();

        uiWidth >>= 1;
        uiHeight >>= 1;
      }
    }
  }

  auto pIrradianceOutput = outputs[m_PinIrradianceData.m_uiOutputIndex];
  if (pIrradianceOutput != nullptr && !pIrradianceOutput->m_TextureHandle.IsInvalidated())
  {
    auto computeScope = ezRenderContext::BeginComputeScope(renderViewContext, "Irradiance");

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindTexture("IrradianceOutput", pIrradianceOutput->m_TextureHandle);
    bindGroup.BindTexture("InputCubemap", m_hInputCubemap);

    UpdateIrradianceConstantBuffer();

    bindGroup.BindBuffer("ezReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

    renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult();
  }
}

ezResult ezReflectionFilterPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fIntensity;
  inout_stream << m_fSaturation;
  inout_stream << m_uiSpecularOutputIndex;
  inout_stream << m_uiIrradianceOutputIndex;
  // inout_stream << m_hInputCubemap; Runtime only property
  return EZ_SUCCESS;
}

ezResult ezReflectionFilterPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fIntensity;
  inout_stream >> m_fSaturation;
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
  m_hInputCubemap = ezGALTextureHandle(ezGAL::ez18_14Id(uiCubemapHandle));
}

void ezReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(ezUInt32 uiMipMapIndex, ezUInt32 uiNumMipMaps, ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  auto constants = ezRenderContext::GetConstantBufferData<ezReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->MipLevel = uiMipMapIndex;
  constants->OutputWidth = uiWidth;
  constants->OutputHeight = uiHeight;
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
}

void ezReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
  auto constants = ezRenderContext::GetConstantBufferData<ezReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
  constants->OutputIndex = m_uiIrradianceOutputIndex;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
