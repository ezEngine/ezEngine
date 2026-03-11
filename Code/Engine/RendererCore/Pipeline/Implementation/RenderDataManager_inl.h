#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

template <typename T>
T* ezRenderDataManager::CreateRenderDataForThisFrame(const ezGameObject* pOwner) const
{
  static_assert(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

  T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_Flags.AddOrRemove(ezRenderData::Flags::Dynamic, pOwner->IsDynamic());
    pRenderData->m_Flags.AddOrRemove(ezRenderData::Flags::FlipWinding, pOwner->GetGlobalTransformSimd().HasMirrorScaling());

    pRenderData->m_vGlobalPosition = pOwner->GetGlobalPosition();

    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}

// static
EZ_FORCE_INLINE void ezRenderDataManager::FillPerInstanceData(ezPerInstanceData& out_perInstanceData, const ezGameObject* pObject, const ezTransform& globalTransform, ezUInt32 uiUniqueID /*= 0*/, const ezColor& color /*= ezColor::White*/, const ezVec4& vCustomData /*= ezVec4(0, 1, 0, 1)*/, float fBoundingSphereRadius /*= 1.0f*/, ezUInt32 uiRandomSeed /*= 0*/)
{
  ezMat4 objectToWorld = globalTransform.GetAsMat4();
  out_perInstanceData.ObjectToWorld = objectToWorld;

  if (globalTransform.HasOnlyUniformScaling())
  {
    out_perInstanceData.ObjectToWorldNormal = objectToWorld;
  }
  else
  {
    ezMat3 mInverse = objectToWorld.GetRotationalPart();
    mInverse.Invert(0.0f).IgnoreResult();
    // we explicitly ignore the return value here (success / failure)
    // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

    out_perInstanceData.ObjectToWorldNormal = mInverse.GetTranspose();
  }

  if (pObject != nullptr)
  {
    out_perInstanceData.BoundingSphereRadius = pObject->GetGlobalBounds().m_fSphereRadius;
    out_perInstanceData.RandomSeed = pObject->GetStableRandomSeed();
  }
  else
  {
    out_perInstanceData.BoundingSphereRadius = fBoundingSphereRadius;
    out_perInstanceData.RandomSeed = uiRandomSeed;
  }

  out_perInstanceData.GameObjectID = uiUniqueID;
  out_perInstanceData.Reserved = 0;

  out_perInstanceData.Color = color;
  out_perInstanceData.CustomData = vCustomData;
}

EZ_FORCE_INLINE ezGALDynamicBufferHandle ezRenderDataManager::GetOrCreateInstanceDataAndFill(const ezComponent& ownerComponent, bool bDynamic, const ezTransform& globalTransform, ezInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiUniqueID /*= 0*/, const ezColor& color /*= ezColor::White*/, const ezVec4& vCustomData /*= ezVec4(0, 1, 0, 1)*/) const
{
  ezGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = GetOrCreateInstanceData(&ownerComponent, bDynamic, hInstanceDataBuffer, inout_instanceDataOffset);
  FillPerInstanceData(instanceData[0], ownerComponent.GetOwner(), globalTransform, uiUniqueID, color, vCustomData);

  return hInstanceDataBuffer;
}

template <typename T>
EZ_ALWAYS_INLINE ezArrayPtr<T> ezRenderDataManager::GetOrCreateCustomInstanceData(ezUInt32 uiCustomDataIndex, const ezComponent* pOwnerComponent, ezGALDynamicBufferHandle& out_hBuffer, ezCustomInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiCount /*= 1*/) const
{
  ezByteArrayPtr data = GetOrCreateCustomInstanceData(uiCustomDataIndex, sizeof(T), pOwnerComponent, out_hBuffer, inout_instanceDataOffset, uiCount);
  return ezArrayPtr<T>(reinterpret_cast<T*>(data.GetPtr()), data.GetCount() / sizeof(T));
}

template <typename T>
EZ_FORCE_INLINE ezGALDynamicBufferHandle ezRenderDataManager::GetOrCreateCustomInstanceDataAndFill(ezUInt32 uiCustomDataIndex, const ezComponent& ownerComponent, ezCustomInstanceDataOffset& inout_instanceDataOffset, const T& data) const
{
  ezGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = GetOrCreateCustomInstanceData<T>(uiCustomDataIndex, &ownerComponent, hInstanceDataBuffer, inout_instanceDataOffset);
  instanceData[0] = data;

  return hInstanceDataBuffer;
}

EZ_ALWAYS_INLINE ezGALDynamicBufferHandle ezRenderDataManager::GetCustomInstanceDataBuffer(ezUInt32 uiCustomDataIndex) const
{
  return m_Buffers[uiCustomDataIndex];
}
