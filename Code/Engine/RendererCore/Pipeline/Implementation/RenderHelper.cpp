#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderHelper.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Context/Context.h>

// static
void ezRenderHelper::SetMaterialState(ezGALContext* pContext, const ezMaterialResourceHandle& hMaterial)
{
  ezResourceLock<ezMaterialResource> pMaterial(hMaterial);

  ezShaderManager::SetActiveShader(pMaterial->GetShader(), pContext);
}

// static 
void ezRenderHelper::DrawMeshBuffer(ezGALContext* pContext, const ezMeshBufferResourceHandle& hMeshBuffer,
  ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiInstanceCount)
{
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