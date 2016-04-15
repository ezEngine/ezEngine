#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>

void ezRenderContext::SetMaterialState(const ezMaterialResourceHandle& hMaterial)
{
  if (!hMaterial.IsValid())
    return;

  ezHybridArray<ezMaterialResource*, 16> materialHierarchy;

  ezMaterialResourceHandle hCurrentMaterial = hMaterial;
  
  while (true)
  {
    ezMaterialResource* pMaterial = ezResourceManager::BeginAcquireResource(hCurrentMaterial);

    materialHierarchy.PushBack(pMaterial);

    const ezMaterialResourceHandle& hParentMaterial = pMaterial->GetDescriptor().m_hBaseMaterial;
    if (!hParentMaterial.IsValid() || hParentMaterial == hCurrentMaterial)
      break;

    hCurrentMaterial = hParentMaterial;
  }

  ezShaderResourceHandle hShader;

  // set state of parent material first
  for (ezUInt32 i = materialHierarchy.GetCount(); i-- > 0; )
  {
    ezMaterialResource* pMaterial = materialHierarchy[i];
    const ezMaterialResourceDescriptor& desc = pMaterial->GetDescriptor();

    if (desc.m_hShader.IsValid())
      hShader = desc.m_hShader;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      SetShaderPermutationVariable(permutationVar.m_Name, permutationVar.m_Value);
    }

    for (const auto& shaderConstant : desc.m_ShaderConstants)
    {
      SetMaterialParameter(shaderConstant.m_Name, shaderConstant.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureBindings)
    {
      ezTempHashedString name(textureBinding.m_Name);

      ezResourceLock<ezTextureResource> pTexture(textureBinding.m_Value, ezResourceAcquireMode::AllowFallback);
      ezGALResourceViewHandle hResourceView = ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture());
      ezGALSamplerStateHandle hSamplerState = pTexture->GetGALSamplerState();

      for (int i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
      {
        BindTexture((ezGALShaderStage::Enum)i, name, hResourceView, hSamplerState);
      }
    }

    ezResourceManager::EndAcquireResource(pMaterial);
  }

  // Always bind the shader so that in case of an invalid shader the drawcall is skipped later.
  // Otherwise we will render with the shader of the previous material which can lead to strange behavior.
  BindShader(hShader);
}

ezRenderContext::Statistics ezRenderContext::GetAndResetStatistics()
{
  ezRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
}

ezResult ezRenderContext::ApplyContextStates(bool bForce)
{
  ezShaderPermutationResource* pShaderPermutation = nullptr;

  bool bRebuildVertexDeclaration = m_StateFlags.IsAnySet(ezRenderContextFlags::ShaderStateChanged | ezRenderContextFlags::MeshBufferBindingChanged);

  if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::ShaderStateChanged))
  {
    m_hActiveGALShader.Invalidate();

    m_StateFlags.Remove(ezRenderContextFlags::ShaderStateValid);
    m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::ConstantBufferBindingChanged);

    if (!m_hActiveShader.IsValid())
      return EZ_FAILURE;

    ezResourceLock<ezShaderResource> pShader(m_hActiveShader, ezResourceAcquireMode::AllowFallback);

    if (!pShader->IsShaderValid())
      return EZ_FAILURE;

    if (m_Topology == ezGALPrimitiveTopology::Lines)
    {
      m_PermutationVariables["TOPOLOGY_LINES"] = "1";
    }
    else
    {
      m_PermutationVariables["TOPOLOGY_LINES"] = "0";
    }

    m_PermGenerator.Clear();
    for (auto itPerm = m_PermutationVariables.GetIterator(); itPerm.IsValid(); ++itPerm)
      m_PermGenerator.AddPermutation(itPerm.Key().GetData(), itPerm.Value().GetData());

    m_PermGenerator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

    EZ_ASSERT_DEV(m_PermGenerator.GetPermutationCount() == 1, "Invalid shader setup");

    ezHybridArray<ezPermutationGenerator::PermutationVar, 16> UsedPermVars;
    m_PermGenerator.GetPermutation(0, UsedPermVars);

    m_hActiveShaderPermutation = PreloadSingleShaderPermutation(m_hActiveShader, UsedPermVars, ezTime::Seconds(0.0));

    if (!m_hActiveShaderPermutation.IsValid())
      return EZ_FAILURE;

    pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    if (!pShaderPermutation->IsShaderValid())
    {
      ezResourceManager::EndAcquireResource(pShaderPermutation);
      return EZ_FAILURE;
    }

    m_hActiveGALShader = pShaderPermutation->GetGALShader();
    EZ_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

    m_pGALContext->SetShader(m_hActiveGALShader);

    // Set render state from shader (unless they are all deactivated)
    if (!m_ShaderBindFlags.AreAllSet(ezShaderBindFlags::NoBlendState | ezShaderBindFlags::NoRasterizerState | ezShaderBindFlags::NoDepthStencilState))
    {
      if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoBlendState))
        m_pGALContext->SetBlendState(pShaderPermutation->GetBlendState());

      if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoRasterizerState))
        m_pGALContext->SetRasterizerState(pShaderPermutation->GetRasterizerState());

      if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoDepthStencilState))
        m_pGALContext->SetDepthStencilState(pShaderPermutation->GetDepthStencilState());
    }

    m_StateFlags.Remove(ezRenderContextFlags::ShaderStateChanged);
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateValid);
  }

  if ((bForce || m_StateFlags.IsSet(ezRenderContextFlags::TextureBindingChanged)) && m_hActiveShaderPermutation.IsValid())
  {
    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage);

      if (pBin == nullptr)
        continue;

      ApplyTextureBindings((ezGALShaderStage::Enum) stage, pBin);
    }

    m_StateFlags.Remove(ezRenderContextFlags::TextureBindingChanged);
  }

  UploadGlobalConstants();

  if ((bForce || m_StateFlags.IsSet(ezRenderContextFlags::ConstantBufferBindingChanged) || (s_LastMaterialParamModification > m_uiLastMaterialCBSync)) && m_hActiveShaderPermutation.IsValid())
  {
    m_uiLastMaterialCBSync = s_LastMaterialParamModification;

    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage);

      if (pBin == nullptr)
        continue;

      ApplyConstantBufferBindings(pBin);
    }

    m_StateFlags.Remove(ezRenderContextFlags::ConstantBufferBindingChanged);
  }

  if (pShaderPermutation != nullptr)
    ezResourceManager::EndAcquireResource(pShaderPermutation);

  if (bForce || bRebuildVertexDeclaration || m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferBindingChanged))
  {
    if (m_hActiveGALShader.IsInvalidated())
      return EZ_FAILURE;

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferBindingChanged))
    {
      m_pGALContext->SetPrimitiveTopology(m_Topology);
      m_pGALContext->SetVertexBuffer(0, m_hVertexBuffer);

      if (!m_hIndexBuffer.IsInvalidated())
        m_pGALContext->SetIndexBuffer(m_hIndexBuffer);
    }

    ezGALVertexDeclarationHandle hVertexDeclaration;
    if (BuildVertexDeclaration(m_hActiveGALShader, *m_pVertexDeclarationInfo, hVertexDeclaration).Failed())
      return EZ_FAILURE;

    m_pGALContext->SetVertexDeclaration(hVertexDeclaration);

    m_StateFlags.Remove(ezRenderContextFlags::MeshBufferBindingChanged);
  }

  return EZ_SUCCESS;
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderHelper);

