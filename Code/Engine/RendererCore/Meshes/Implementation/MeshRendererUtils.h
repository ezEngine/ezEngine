#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

namespace ezInternal
{
  EZ_FORCE_INLINE void FillPerInstanceData(ezPerInstanceData& perInstanceData, const ezMeshRenderData* pRenderData)
  {
    ezMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

    perInstanceData.ObjectToWorld = objectToWorld;

    if (pRenderData->m_uiUniformScale)
    {
      perInstanceData.ObjectToWorldNormal = objectToWorld;
    }
    else
    {
      ezMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f);
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

      ezShaderTransform shaderT;
      shaderT = mInverse.GetTranspose();
      perInstanceData.ObjectToWorldNormal = shaderT;
    }

    perInstanceData.BoundingSphereRadius = pRenderData->m_GlobalBounds.m_fSphereRadius;
    perInstanceData.GameObjectID = pRenderData->m_uiUniqueID;
    perInstanceData.VertexColorAccessData = 0;
    perInstanceData.Color = pRenderData->m_Color;
  }
}
