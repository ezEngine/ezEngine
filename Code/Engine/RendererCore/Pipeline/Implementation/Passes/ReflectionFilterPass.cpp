#include <RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

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
    EZ_MEMBER_PROPERTY("OutputIndex", m_uiOutputIndex),
    EZ_ACCESSOR_PROPERTY("InputCubemap", GetInputCubemap, SetInputCubemap)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezReflectionFilterPass::ezReflectionFilterPass()
  : ezRenderPipelinePass("ReflectionFilterPass")
  , m_fIntensity(1.0f)
  , m_fSaturation(1.0f)
  , m_uiOutputIndex(0)
{
  {
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

  ezGALPass* pGALPass = pDevice->BeginPass(GetName());
  EZ_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  {
    auto pCommandEncoder = ezRenderContext::BeginRenderingScope(pGALPass, renderViewContext, ezGALRenderingSetup(), "MipMaps + Reflection");

    pCommandEncoder->GenerateMipMaps(pDevice->GetDefaultResourceView(m_hInputCubemap));

    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated())
    {
      ezUInt32 uiNumMipMaps = pInputCubemap->GetDescription().m_uiMipLevelCount;

      ezBoundingBoxu32 srcBox;
      srcBox.m_vMin = ezVec3U32(0);
      srcBox.m_vMax = ezVec3U32(pInputCubemap->GetDescription().m_uiWidth, pInputCubemap->GetDescription().m_uiHeight, 1);

      for (ezUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (ezUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          ezGALTextureSubresource destSubResource{uiMipMapIndex, m_uiOutputIndex * 6 + uiFaceIndex};
          ezGALTextureSubresource srcSubResource{uiMipMapIndex, uiFaceIndex};

          pCommandEncoder->CopyTextureRegion(pFilteredSpecularOutput->m_TextureHandle, destSubResource, ezVec3U32(0), m_hInputCubemap, srcSubResource, srcBox);
        }

        srcBox.m_vMax.x >>= 1;
        srcBox.m_vMax.y >>= 1;
      }
    }
  }

  auto pIrradianceOutput = outputs[m_PinIrradianceData.m_uiOutputIndex];
  if (pIrradianceOutput != nullptr && !pIrradianceOutput->m_TextureHandle.IsInvalidated())
  {
    auto pCommandEncoder = ezRenderContext::BeginComputeScope(pGALPass, renderViewContext, "Irradiance");

    ezGALUnorderedAccessViewHandle hIrradianceOutput;
    {
      ezGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pIrradianceOutput->m_TextureHandle;

      hIrradianceOutput = pDevice->CreateUnorderedAccessView(desc);
    }
    renderViewContext.m_pRenderContext->BindUAV("IrradianceOutput", hIrradianceOutput);

    renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));

    UpdateIrradianceConstantBuffer();

    renderViewContext.m_pRenderContext->BindConstantBuffer("ezReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

    renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult();
  }
}

ezUInt32 ezReflectionFilterPass::GetInputCubemap() const
{
  return m_hInputCubemap.GetInternalID().m_Data;
}

void ezReflectionFilterPass::SetInputCubemap(ezUInt32 uiCubemapHandle)
{
  m_hInputCubemap = ezGALTextureHandle(ezGAL::ez18_14Id(uiCubemapHandle));
}

void ezReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
  auto constants = ezRenderContext::GetConstantBufferData<ezReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
  constants->OutputIndex = m_uiOutputIndex;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
