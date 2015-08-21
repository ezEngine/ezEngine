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

  ezResourceLock<ezMaterialResource> pMaterial(hMaterial);

  const ezMaterialResourceDescriptor&  md = pMaterial->GetDescriptor();

  if (md.m_hBaseMaterial.IsValid() && md.m_hBaseMaterial != hMaterial)
    SetMaterialState(md.m_hBaseMaterial);

  if (md.m_hShader.IsValid())
    BindShader(md.m_hShader);

  for (const auto& pv : md.m_PermutationVars)
  {
    SetShaderPermutationVariable(pv.m_Name, pv.m_Value);
  }

  for (const auto& sc : md.m_ShaderConstants)
  {
    SetMaterialParameter(ezTempHashedString(sc.m_NameHash), sc.m_Value);
  }

  for (const auto& tb : md.m_TextureBindings)
  {
    BindTexture(ezTempHashedString(tb.m_NameHash), tb.m_Value);
  }
}

ezRenderContext::Statistics ezRenderContext::GetAndResetStatistics()
{
  ezRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
}

// static 
void ezRenderContext::DrawMeshBuffer(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed())
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return;
  }

  EZ_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive (%d) can't be larger than number of primitives (%d)", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = ezMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  EZ_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");


  uiPrimitiveCount *= 3;
  uiFirstPrimitive *= 3;

  if (uiInstanceCount > 1)
  {
    if (m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferHasIndexBuffer))
    {
      m_pGALContext->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      m_pGALContext->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferHasIndexBuffer))
    {
      m_pGALContext->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      m_pGALContext->Draw(uiPrimitiveCount, uiFirstPrimitive);
    }
  }
}

ezResult ezRenderContext::ApplyContextStates(bool bForce)
{
  ezShaderPermutationResource* pShaderPermutation = nullptr;

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

  if ((bForce || m_StateFlags.IsSet(ezRenderContextFlags::ConstantBufferBindingChanged)) && m_hActiveShaderPermutation.IsValid())
  {
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

  if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferBindingChanged))
  {
    if (!m_hMeshBuffer.IsValid())
      return EZ_FAILURE;

    if (m_hActiveGALShader.IsInvalidated())
      return EZ_FAILURE;

    ezResourceLock<ezMeshBufferResource> pMeshBuffer(m_hMeshBuffer);
    m_uiMeshBufferPrimitiveCount = pMeshBuffer->GetPrimitiveCount();

    m_pGALContext->SetVertexBuffer(0, pMeshBuffer->GetVertexBuffer());

    ezGALBufferHandle hIndexBuffer = pMeshBuffer->GetIndexBuffer();

    // store whether we have an index buffer (needed during drawcalls)
    m_StateFlags.AddOrRemove(ezRenderContextFlags::MeshBufferHasIndexBuffer, !hIndexBuffer.IsInvalidated());

    if (!hIndexBuffer.IsInvalidated())
      m_pGALContext->SetIndexBuffer(hIndexBuffer);

    m_pGALContext->SetPrimitiveTopology(ezGALPrimitiveTopology::Triangles);

    ezGALVertexDeclarationHandle hVertexDeclaration;
    if (BuildVertexDeclaration(m_hActiveGALShader, pMeshBuffer->GetVertexDeclaration(), hVertexDeclaration).Failed())
      return EZ_FAILURE;

    m_pGALContext->SetVertexDeclaration(hVertexDeclaration);

    m_StateFlags.Remove(ezRenderContextFlags::MeshBufferBindingChanged);
  }

  return EZ_SUCCESS;
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderHelper);

