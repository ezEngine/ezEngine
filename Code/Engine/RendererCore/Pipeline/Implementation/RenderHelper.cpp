#include <RendererCore/PCH.h>
#include <RendererCore/RendererCore.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>

void ezRendererCore::SetMaterialState(const ezMaterialResourceHandle& hMaterial)
{
  ezResourceLock<ezMaterialResource> pMaterial(hMaterial);

  const ezMaterialResourceDescriptor&  md = pMaterial->GetDescriptor();

  if (md.m_hBaseMaterial.IsValid())
    SetMaterialState(md.m_hBaseMaterial);

  if (md.m_hShader.IsValid())
    SetActiveShader(md.m_hShader);

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

ezUInt32 ezRendererCore::RetrieveFailedDrawcalls()
{
  return m_ContextState.m_uiFailedDrawcalls;
}

// static 
void ezRendererCore::DrawMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer, ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed())
  {
    m_ContextState.m_uiFailedDrawcalls++;
    return;
  }

  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

  EZ_ASSERT_DEV(uiFirstPrimitive < pMeshBuffer->GetPrimitiveCount(), "Invalid primitive range: first primitive (%d) can't be larger than number of primitives (%d)", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = ezMath::Min(uiPrimitiveCount, pMeshBuffer->GetPrimitiveCount() - uiFirstPrimitive);
  EZ_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  m_pGALContext->SetVertexBuffer(0, pMeshBuffer->GetVertexBuffer());

  ezGALBufferHandle hIndexBuffer = pMeshBuffer->GetIndexBuffer();
  if (!hIndexBuffer.IsInvalidated())
    m_pGALContext->SetIndexBuffer(hIndexBuffer);

  m_pGALContext->SetPrimitiveTopology(ezGALPrimitiveTopology::Triangles);
  uiPrimitiveCount *= 3;
  uiFirstPrimitive *= 3;

  {
    m_pGALContext->SetVertexDeclaration(GetVertexDeclaration(m_ContextState.m_hActiveGALShader, pMeshBuffer->GetVertexDeclaration()));
  }

  if (uiInstanceCount > 1)
  {
    if (!hIndexBuffer.IsInvalidated())
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
    if (!hIndexBuffer.IsInvalidated())
    {
      m_pGALContext->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      m_pGALContext->Draw(uiPrimitiveCount, uiFirstPrimitive);
    }
  }
}

ezResult ezRendererCore::ApplyContextStates(bool bForce)
{
  // make sure the internal state is up to date
  SetShaderContextState(bForce);

  if (!m_ContextState.m_bShaderStateValid)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderHelper);

