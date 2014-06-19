
namespace ezInternal
{

// static
template <typename VISITOR>
EZ_FORCE_INLINE bool WorldData::TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData /* = nullptr*/)
{
  for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < blocks.GetCount(); ++uiBlockIndex)
  {
    WorldData::Hierarchy::DataBlock& block = blocks[uiBlockIndex];
    ezGameObject::TransformationData* pCurrentData = block.m_pData;
    ezGameObject::TransformationData* pEndData = block.m_pData + block.m_uiCount;

    while (pCurrentData < pEndData)
    {
      if (!VISITOR::Visit(pCurrentData, pUserData))
        return false;

      ++pCurrentData;
    }
  }

  return true;
}

// static
EZ_FORCE_INLINE void WorldData::UpdateWorldTransform(ezGameObject::TransformationData* pData, float fInvDeltaSeconds)
{
  const ezVec3 vPos = *reinterpret_cast<const ezVec3*>(&pData->m_localPosition);
  const ezQuat qRot = pData->m_localRotation;
  const ezVec3 vScale = *reinterpret_cast<const ezVec3*>(&pData->m_localScaling);

  const ezVec3 vOldWorldPos = pData->m_worldTransform.m_vPosition;
  pData->m_worldTransform = ezTransform(vPos, qRot, vScale);
  pData->m_velocity = ((pData->m_worldTransform.m_vPosition - vOldWorldPos) * fInvDeltaSeconds).GetAsDirectionVec4();
}

// static
EZ_FORCE_INLINE void WorldData::UpdateWorldTransformWithParent(ezGameObject::TransformationData* pData, float fInvDeltaSeconds)
{
  const ezVec3 vPos = *reinterpret_cast<const ezVec3*>(&pData->m_localPosition);
  const ezQuat qRot = pData->m_localRotation;
  const ezVec3 vScale = *reinterpret_cast<const ezVec3*>(&pData->m_localScaling);

  const ezVec3 vOldWorldPos = pData->m_worldTransform.m_vPosition;
  const ezTransform localTransform(vPos, qRot, vScale);
  pData->m_worldTransform.SetGlobalTransform(pData->m_pParentData->m_worldTransform, localTransform);
  pData->m_velocity = ((pData->m_worldTransform.m_vPosition - vOldWorldPos) * fInvDeltaSeconds).GetAsDirectionVec4();
}

}
