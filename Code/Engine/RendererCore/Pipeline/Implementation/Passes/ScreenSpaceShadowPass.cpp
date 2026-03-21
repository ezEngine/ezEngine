#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Lights/ScreenSpaceShadowDataProvider.h>
#include <RendererCore/Pipeline/Passes/ScreenSpaceShadowPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScreenSpaceShadowPass, 5, ezRTTIDefaultAllocator<ezScreenSpaceShadowPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)
      ->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 1.0f)),
    EZ_MEMBER_PROPERTY("MaxSteps", m_uiMaxSteps)
      ->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(4, 64)),
    EZ_MEMBER_PROPERTY("SurfaceThickness", m_fSurfaceThickness)
      ->AddAttributes(new ezDefaultValueAttribute(0.02f), new ezClampValueAttribute(0.001f, 0.5f)),
    EZ_MEMBER_PROPERTY("ShadowIntensity", m_fShadowIntensity)
      ->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
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

ezScreenSpaceShadowPass::ezScreenSpaceShadowPass()
  : ezRenderPipelinePass("ScreenSpaceShadowPass", true)
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/ScreenSpaceShadow.ezShader");
  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezScreenSpaceShadowConstants>();
}

ezScreenSpaceShadowPass::~ezScreenSpaceShadowPass()
{
  DestroyResources();
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

bool ezScreenSpaceShadowPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  if (!inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    ezLog::Error("No depth input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezScreenSpaceShadowPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthInput == nullptr)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALTexture* pDepthTex = pDevice->GetTexture(pDepthInput->m_TextureHandle);
  const ezUInt32 uiWidth = pDepthTex->GetDescription().m_uiWidth;
  const ezUInt32 uiHeight = pDepthTex->GetDescription().m_uiHeight;
  const ezUInt32 uiArraySize = pDepthTex->GetDescription().m_uiArraySize;

  EnsureResources(uiWidth, uiHeight, uiArraySize);

  // m_vBrightestDirectionalLightDirection points toward the light (scene to light).
  // Pass it directly so the shader marches toward the light.
  auto pClusteredData = GetPipeline()->GetFrameDataProvider<ezClusteredDataProvider>()->GetData(renderViewContext);
  ezVec3 lightDir = pClusteredData->m_vBrightestDirectionalLightDirection;

  auto pProvider = GetPipeline()->GetFrameDataProvider<ezScreenSpaceShadowDataProvider>();

  if (lightDir.IsZero(0.001f))
  {
    if (pProvider != nullptr)
    {
      pProvider->SetTexture(ezGALTextureHandle());
    }
    return;
  }

  lightDir.Normalize();

  const ezUInt32 dispatchX = (uiWidth + SSS_THREAD_GROUP_SIZE - 1) / SSS_THREAD_GROUP_SIZE;
  const ezUInt32 dispatchY = (uiHeight + SSS_THREAD_GROUP_SIZE - 1) / SSS_THREAD_GROUP_SIZE;

  // Fill constant buffer
  {
    auto* cb = ezRenderContext::GetConstantBufferData<ezScreenSpaceShadowConstants>(m_hConstantBuffer);
    cb->SSSLightDirectionWS = lightDir;
    cb->SSSMaxRayDistance = m_fMaxRayDistance;
    cb->SSSMaxSteps = m_uiMaxSteps;
    cb->SSSSurfaceThickness = m_fSurfaceThickness;
    cb->SSSShadowIntensity = m_fShadowIntensity;
    cb->SSSFrameIndex = m_uiFrameIndex;
    cb->SSSTexelSize = ezVec2(1.0f / uiWidth, 1.0f / uiHeight);
    cb->SSSResolutionX = uiWidth;
    cb->SSSResolutionY = uiHeight;
  }

  // Ray march pass: writes raw shadow to m_hShadowTexture
  {
    auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "SS Shadow Trace");

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("ezScreenSpaceShadowConstants", m_hConstantBuffer);
    bindGroup.BindTexture("DepthTexture", pDepthInput->m_TextureHandle);
    bindGroup.BindTexture("OutputTexture", m_hShadowTexture);

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->Dispatch(dispatchX, dispatchY, 1).IgnoreResult();
  }

  // Publish the final shadow texture
  if (pProvider != nullptr)
  {
    pProvider->SetTexture(m_hShadowTexture);
  }

  // Set the ScreenSpaceShadowEnabled flag
  {
    auto* pConstants = ezRenderContext::GetConstantBufferData<ezClusteredDataConstants>(pClusteredData->m_hConstantBuffer);
    pConstants->ScreenSpaceShadowEnabled = 1;
  }

  ++m_uiFrameIndex;
}

void ezScreenSpaceShadowPass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pProvider = GetPipeline()->GetFrameDataProvider<ezScreenSpaceShadowDataProvider>();
  if (pProvider != nullptr)
  {
    pProvider->SetTexture(ezGALTextureHandle());
  }
}

void ezScreenSpaceShadowPass::EnsureResources(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiArraySize)
{
  if (m_uiWidth == uiWidth && m_uiHeight == uiHeight && !m_hShadowTexture.IsInvalidated())
    return;

  DestroyResources();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALTextureCreationDescription desc;
  desc.m_uiWidth = uiWidth;
  desc.m_uiHeight = uiHeight;
  desc.m_Format = ezGALResourceFormat::RFloat;
  desc.m_Type = ezGALTextureType::Texture2DArray;
  desc.m_uiArraySize = uiArraySize;
  desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::UnorderedAccess;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hShadowTexture = pDevice->CreateTexture(desc);

  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
}

void ezScreenSpaceShadowPass::DestroyResources()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (!m_hShadowTexture.IsInvalidated())
    pDevice->DestroyTexture(m_hShadowTexture);

  m_hShadowTexture.Invalidate();
  m_uiWidth = 0;
  m_uiHeight = 0;
}

ezResult ezScreenSpaceShadowPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fMaxRayDistance;
  inout_stream << m_uiMaxSteps;
  inout_stream << m_fSurfaceThickness;
  inout_stream << m_fShadowIntensity;
  return EZ_SUCCESS;
}

ezResult ezScreenSpaceShadowPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  if (uiVersion >= 5)
  {
    inout_stream >> m_fMaxRayDistance;
    inout_stream >> m_uiMaxSteps;
    inout_stream >> m_fSurfaceThickness;
    inout_stream >> m_fShadowIntensity;
  }
  else if (uiVersion == 4)
  {
    inout_stream >> m_fMaxRayDistance;
    inout_stream >> m_uiMaxSteps;
    inout_stream >> m_fSurfaceThickness;
    inout_stream >> m_fShadowIntensity;

    // v4 had denoise properties that are no longer used
    bool bIgnoreBool;
    float fIgnoreFloat;
    inout_stream >> bIgnoreBool;
    inout_stream >> fIgnoreFloat;
    inout_stream >> fIgnoreFloat;
  }
  else if (uiVersion == 3)
  {
    inout_stream >> m_fMaxRayDistance;
    inout_stream >> m_uiMaxSteps;
    inout_stream >> m_fSurfaceThickness;
    inout_stream >> m_fShadowIntensity;
    // Denoise properties didn't exist in v3, use defaults
  }
  else if (uiVersion == 2)
  {
    float fIgnore;
    ezUInt32 uiIgnore;
    inout_stream >> m_fMaxRayDistance;
    inout_stream >> uiIgnore;
    m_uiMaxSteps = 16;
    inout_stream >> m_fSurfaceThickness;
    inout_stream >> m_fShadowIntensity;
    inout_stream >> fIgnore;
    inout_stream >> fIgnore;
    inout_stream >> fIgnore;
  }
  else
  {
    float fIgnore;
    inout_stream >> fIgnore;
    inout_stream >> fIgnore;
    inout_stream >> fIgnore;
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ScreenSpaceShadowPass);
