#include <RendererCore/PCH.h>
#include <RendererCore/RendererCore.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Context/Context.h>

ezMap<ezGALContext*, ezRendererCore::ContextState> ezRendererCore::s_ContextState;

// static
void ezRendererCore::SetMaterialState(ezGALContext* pContext, const ezMaterialResourceHandle& hMaterial)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ezResourceLock<ezMaterialResource> pMaterial(hMaterial);

  ezRendererCore::SetActiveShader(pMaterial->GetShader(), pContext);
}

// static 
void ezRendererCore::DrawMeshBuffer(ezGALContext* pContext, const ezMeshBufferResourceHandle& hMeshBuffer,
  ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiInstanceCount)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  if (ApplyContextStates(pContext).Failed())
  {
    ezLog::Error("Drawcall failed, context is in an invalid state");
    /// \todo Log (better) that a drawcall failed ?
    return;
  }

  EZ_ASSERT(uiFirstPrimitive < uiPrimitiveCount, "Invalid primitive range: first primitive (%d) can't be larger than number of primitives (%d)", uiFirstPrimitive, uiPrimitiveCount);

  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

  uiPrimitiveCount = ezMath::Min(uiPrimitiveCount, pMeshBuffer->GetPrimitiveCount() - uiFirstPrimitive);
  EZ_ASSERT(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  pContext->SetVertexBuffer(0, pMeshBuffer->GetVertexBuffer());

  ezGALBufferHandle hIndexBuffer = pMeshBuffer->GetIndexBuffer();
  if (!hIndexBuffer.IsInvalidated())
    pContext->SetIndexBuffer(hIndexBuffer);

  pContext->SetPrimitiveTopology(ezGALPrimitiveTopology::Triangles);
  uiPrimitiveCount *= 3;
  uiFirstPrimitive *= 3;

  {
    ContextState& state = s_ContextState[pContext];
    pContext->SetVertexDeclaration(GetVertexDeclaration(state.m_hActiveGALShader, pMeshBuffer->GetVertexDeclaration()));
  }

  if (uiInstanceCount > 1)
  {
    if (!hIndexBuffer.IsInvalidated())
    {
      pContext->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      pContext->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (!hIndexBuffer.IsInvalidated())
    {
      pContext->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      pContext->Draw(uiPrimitiveCount, uiFirstPrimitive);
    }
  }
}

void ezRendererCore::BindTexture(ezGALContext* pContext, const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& cs = s_ContextState[pContext];

  cs.m_BoundTextures[sSlotName.GetHash()] = hTexture;

  cs.m_bTextureBindingsChanged = true;
}

ezResult ezRendererCore::ApplyContextStates(ezGALContext* pContext, bool bForce)
{
  if (pContext == nullptr)
    pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

  ContextState& state = s_ContextState[pContext];

  // make sure the internal state is up to date
  SetShaderContextState(pContext, state, bForce);

  if (!state.m_bShaderStateValid)
    return EZ_FAILURE;


  return EZ_SUCCESS;
}


