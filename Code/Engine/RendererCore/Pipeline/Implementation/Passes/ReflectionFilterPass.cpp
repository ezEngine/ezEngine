#include <RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

//#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BloomConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReflectionFilterPass, 1, ezRTTIDefaultAllocator<ezReflectionFilterPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FilteredSpecular", m_PinFilteredSpecular),
    EZ_MEMBER_PROPERTY("AvgLuminance", m_PinAvgLuminance),
    EZ_MEMBER_PROPERTY("IrradianceData", m_PinIrradianceData),
    EZ_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
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
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Bloom.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load bloom shader!");
  }

  {
    // m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezBloomConstants>();
  }
}

ezReflectionFilterPass::~ezReflectionFilterPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool ezReflectionFilterPass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
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

void ezReflectionFilterPass::Execute(const ezRenderViewContext& renderViewContext,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto pInputCubemap = pDevice->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
  {
    return;
  }

  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
  if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated())
  {
    pGALContext->GenerateMipMaps(pDevice->GetDefaultResourceView(m_hInputCubemap));
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

void ezReflectionFilterPass::UpdateConstantBuffer(ezVec2 pixelSize, const ezColor& tintColor)
{
  /*ezBloomConstants* constants = ezRenderContext::GetConstantBufferData<ezBloomConstants>(m_hConstantBuffer);
  constants->PixelSize = pixelSize;
  constants->BloomThreshold = m_fThreshold;
  constants->BloomIntensity = m_fIntensity;

  constants->TintColor = tintColor;*/
}
