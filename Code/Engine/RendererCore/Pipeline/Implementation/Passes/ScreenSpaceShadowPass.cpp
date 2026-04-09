#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Pipeline/Passes/ScreenSpaceShadowPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Shaders/Pipeline/ScreenSpaceShadowConstants.h>
#include <Shaders/Pipeline/bend_sss/bend_sss_cpu.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScreenSpaceShadowPass, 1, ezRTTIDefaultAllocator<ezScreenSpaceShadowPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("SurfaceThickness", m_fSurfaceThickness)->AddAttributes(new ezDefaultValueAttribute(0.005f), new ezClampValueAttribute(0.001f, 0.5f)),
    EZ_MEMBER_PROPERTY("ShadowContrast", m_fShadowContrast)->AddAttributes(new ezDefaultValueAttribute(4.0f), new ezClampValueAttribute(1.0f, 10.0f)),
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
  ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hDepthSamplerState);

  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

bool ezScreenSpaceShadowPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  if (auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex])
  {
    if (!pDepthInput->m_TextureFlags.IsSet(ezGALTextureUsageFlags::ShaderResource))
    {
      ezLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    if (pDepthInput->m_SampleCount != ezGALMSAASampleCount::None)
    {
      ezLog::Error("'{0}' input must be resolved", GetName());
      return false;
    }

    ezGALTextureCreationDescription desc = *pDepthInput;
    desc.m_Format = ezGALResourceFormat::RUByteNormalized;
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::UnorderedAccess;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezScreenSpaceShadowPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pDepthInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  bool bRendered = false;

  auto batchList = GetPipeline()->GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Light);
  const ezUInt32 uiBatchCount = batchList.GetBatchCount();
  for (ezUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const ezRenderDataBatch& batch = batchList.GetBatch(i);

    for (auto it = batch.GetIterator<ezRenderData>(); it.IsValid(); ++it)
    {
      const auto pDirLight = ezDynamicCast<const ezDirectionalLightRenderData*>(it);
      if (pDirLight == nullptr)
      {
        // Directional lights come first in the list, so we can stop as soon as we encounter a non-directional light.
        return;
      }

      if (!pDirLight->m_bScreenSpaceShadows)
        continue;

      if (bRendered)
      {
        ezLog::Warning("Multiple directional lights with screen space shadows are not yet supported. Only the first one will be rendered.");
        return;
      }

      CreateSamplerState();

      ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
      const ezGALTexture* pDepthTex = pDevice->GetTexture(pDepthInput->m_TextureHandle);
      const ezUInt32 uiWidth = pDepthTex->GetDescription().m_uiWidth;
      const ezUInt32 uiHeight = pDepthTex->GetDescription().m_uiHeight;

      ezVec3 lightDir = pDirLight->m_vDirection;
      lightDir.Normalize();

      const ezUInt32 uiEyeCount = renderViewContext.m_pCamera->IsStereoscopic() ? 2 : 1;
      for (ezUInt32 uiEyeIndex = 0; uiEyeIndex < uiEyeCount; ++uiEyeIndex)
      {
        ezVec4 projectedLightDir = renderViewContext.m_pViewData->m_ViewProjectionMatrix[uiEyeIndex] * ezVec4(lightDir, 0.0f);

        ezVec2I32 viewportSize = ezVec2I32::Make(uiWidth, uiHeight);
        ezVec2I32 minRenderBounds = ezVec2I32::Make(0, 0);
        ezVec2I32 maxRenderBounds = viewportSize;

        Bend::DispatchList dispatchList = Bend::BuildDispatchList(projectedLightDir.GetData(), viewportSize.GetData(), minRenderBounds.GetData(), maxRenderBounds.GetData(), false, WAVE_SIZE);

        // Ray march pass: writes raw shadow to output texture
        {
          auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "ScreenSpaceShadow");

          ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
          bindGroup.BindBuffer("ezScreenSpaceShadowConstants", m_hConstantBuffer);
          bindGroup.BindTexture("DepthTexture", pDepthInput->m_TextureHandle);
          bindGroup.BindSampler("DepthTextureSampler", m_hDepthSamplerState);
          bindGroup.BindTexture("OutputTexture", pOutput->m_TextureHandle);

          renderViewContext.m_pRenderContext->BindShader(m_hShader);

          for (int i = 0; i < dispatchList.DispatchCount; ++i)
          {
            auto& dispatch = dispatchList.Dispatch[i];

            // Fill constant buffer
            {
              auto cb = ezRenderContext::GetConstantBufferData<ezScreenSpaceShadowConstants>(m_hConstantBuffer);
              cb->LightCoordinate = ezVec4(dispatchList.LightCoordinate_Shader[0], dispatchList.LightCoordinate_Shader[1], dispatchList.LightCoordinate_Shader[2], dispatchList.LightCoordinate_Shader[3]);
              cb->WaveOffset = ezVec2I32(dispatch.WaveOffset_Shader[0], dispatch.WaveOffset_Shader[1]);
              cb->InvDepthTextureSize = ezVec2(1.0f / uiWidth, 1.0f / uiHeight);
              cb->SurfaceThickness = m_fSurfaceThickness;
              cb->ShadowContrast = m_fShadowContrast;
              cb->EyeIndex = uiEyeIndex;
            }

            renderViewContext.m_pRenderContext->Dispatch(dispatch.WaveCount[0], dispatch.WaveCount[1], dispatch.WaveCount[2]).IgnoreResult();
          }
        }
      }

      bRendered = true;
    }
  }
}

ezResult ezScreenSpaceShadowPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fSurfaceThickness;
  inout_stream << m_fShadowContrast;
  return EZ_SUCCESS;
}

ezResult ezScreenSpaceShadowPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  inout_stream >> m_fSurfaceThickness;
  inout_stream >> m_fShadowContrast;

  return EZ_SUCCESS;
}

void ezScreenSpaceShadowPass::CreateSamplerState()
{
  if (m_hDepthSamplerState.IsInvalidated())
  {
    ezGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = ezGALTextureFilterMode::Point;
    desc.m_MagFilter = ezGALTextureFilterMode::Point;
    desc.m_MipFilter = ezGALTextureFilterMode::Point;
    desc.m_AddressU = ezImageAddressMode::ClampBorder;
    desc.m_AddressV = ezImageAddressMode::ClampBorder;
    desc.m_AddressW = ezImageAddressMode::ClampBorder;
    desc.m_BorderColor = ezColor::White;

    m_hDepthSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ScreenSpaceShadowPass);
