#include <RendererCorePCH.h>

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
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezReflectionFilterPass::ezReflectionFilterPass()
  : ezRenderPipelinePass("ReflectionFilterPass")
  , m_fIntensity(1.0f)
  , m_fSaturation(1.0f)
  , m_uiIrradianceOutputIndex(0)
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

  ezGALPass* pGALPass = pDevice->BeginPass(GetName());
  EZ_SCOPE_EXIT(
    pDevice->EndPass(pGALPass);
    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  {
    auto pCommandEncoder = ezRenderContext::BeginRenderingScope(pGALPass, renderViewContext, ezGALRenderingSetup(), "MipMaps");

    if (pInputCubemap->GetDescription().m_bAllowDynamicMipGeneration)
    {
      pCommandEncoder->GenerateMipMaps(pDevice->GetDefaultResourceView(m_hInputCubemap));
    }
  }

  {

    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated())
    {
      ezUInt32 uiNumMipMaps = pFilteredSpecularOutput->m_Desc.m_uiMipLevelCount; //pInputCubemap->GetDescription().m_uiMipLevelCount;

      ezBoundingBoxu32 srcBox;
      srcBox.m_vMin = ezVec3U32(0);
      srcBox.m_vMax = ezVec3U32(pFilteredSpecularOutput->m_Desc.m_uiWidth, pFilteredSpecularOutput->m_Desc.m_uiHeight, 1);

      renderViewContext.m_pRenderContext->BindConstantBuffer("ezReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);
      renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
      renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));

      for (ezUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (ezUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          ezGALTextureSubresource destSubResource{uiMipMapIndex, m_uiSpecularOutputIndex * 6 + uiFaceIndex};
          ezGALTextureSubresource srcSubResource{uiMipMapIndex, uiFaceIndex};

          ezGALRenderingSetup renderingSetup;

          ezGALRenderTargetViewCreationDescription desc;
          desc.m_hTexture = pFilteredSpecularOutput->m_TextureHandle;
          desc.m_uiMipLevel = uiMipMapIndex;
          desc.m_uiFirstSlice = destSubResource.m_uiArraySlice;
          desc.m_uiSliceCount = 1;

          renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(desc));
          renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, ezRectFloat((float)srcBox.m_vMax.x, (float)srcBox.m_vMax.y), "FilteredSpecular");

          UpdateFilteredSpecularConstantBuffer(uiMipMapIndex, uiNumMipMaps, destSubResource.m_uiArraySlice);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          renderViewContext.m_pRenderContext->EndRendering();
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

    static bool bla = false;
    if (bla)
    {
      auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
      auto hTexture = pFilteredSpecularOutput->m_TextureHandle;
      auto pTexture = pDevice->GetTexture(hTexture);

      bla = false;
      pCommandEncoder->ReadbackTexture(hTexture);
      const ezUInt32 uiNumMipMaps = pTexture->GetDescription().m_uiMipLevelCount;

      ezImageHeader header;
      header.SetImageFormat(ezImageFormat::R16G16B16A16_FLOAT);
      header.SetWidth(pTexture->GetDescription().m_uiWidth);
      header.SetHeight(pTexture->GetDescription().m_uiHeight);
      header.SetNumFaces(6);
      header.SetNumMipLevels(uiNumMipMaps);
      ezImage image;
      image.ResetAndAlloc(header);

      ezHybridArray<ezGALTextureSubresource, 6> sourceSubResources;
      ezHybridArray<ezGALSystemMemoryDescription, 6> memDescriptions;

      for (ezUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (ezUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          ezGALTextureSubresource& subRes = sourceSubResources.ExpandAndGetRef();
          subRes.m_uiArraySlice = uiFaceIndex;
          subRes.m_uiMipLevel = uiMipMapIndex;

          ezGALSystemMemoryDescription& memDesc = memDescriptions.ExpandAndGetRef();
          memDesc.m_pData = image.GetPixelPointer<ezUInt8>(uiMipMapIndex, uiFaceIndex);
          memDesc.m_uiRowPitch = static_cast<ezUInt32>(image.GetRowPitch(uiMipMapIndex));
          memDesc.m_uiSlicePitch = static_cast<ezUInt32>(image.GetDepthPitch(uiMipMapIndex));
        }
      }

      pCommandEncoder->CopyTextureReadbackResult(hTexture, sourceSubResources.GetArrayPtr(), memDescriptions.GetArrayPtr());

      if (image.SaveTo("C://temp//cube3.dds").Failed())
      {
        printf("");
      }
    }


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

ezVec4 GetV4(ezVec3 data)
{
  return ezVec4(data.x, data.y, data.z, 0.0f);
}

void ezReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(ezUInt32 uiMipMapIndex, ezUInt32 uiNumMipMaps, ezUInt32 outputIndex)
{
  ezVec3 vForward[6] = {
    ezVec3(1.0f, 0.0f, 0.0f),
    ezVec3(-1.0f, 0.0f, 0.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, -1.0f),
    ezVec3(0.0f, -1.0f, 0.0f),
    ezVec3(0.0f, 1.0f, 0.0f),
  };

  ezVec3 vUp[6] = {
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 1.0f, 0.0f),
    ezVec3(0.0f, -1.0f, 0.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
  };

  auto constants = ezRenderContext::GetConstantBufferData<ezReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->Forward = GetV4(vForward[outputIndex % 6]);
  constants->Up2 = GetV4(vUp[outputIndex % 6]);
  constants->MipLevel = uiMipMapIndex;
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
  constants->OutputIndex = outputIndex;
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
