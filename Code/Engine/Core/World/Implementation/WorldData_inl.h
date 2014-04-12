
namespace ezInternal
{

// static
EZ_FORCE_INLINE void WorldData::UpdateWorldTransform(ezGameObject::TransformationData* pData)
{
  const ezVec3 vPos = *reinterpret_cast<const ezVec3*>(&pData->m_localPosition);
  const ezQuat qRot = pData->m_localRotation;
  const ezVec3 vScale = *reinterpret_cast<const ezVec3*>(&pData->m_localScaling);

  pData->m_worldTransform = ezTransform(vPos, qRot, vScale);
}

// static
EZ_FORCE_INLINE void WorldData::UpdateWorldTransformWithParent(ezGameObject::TransformationData* pData)
{
  const ezVec3 vPos = *reinterpret_cast<const ezVec3*>(&pData->m_localPosition);
  const ezQuat qRot = pData->m_localRotation;
  const ezVec3 vScale = *reinterpret_cast<const ezVec3*>(&pData->m_localScaling);

  const ezTransform localTransform(vPos, qRot, vScale);  
  pData->m_worldTransform.SetGlobalTransform(pData->m_pParentData->m_worldTransform, localTransform);
}

// static
template <typename UPDATER>
inline void WorldData::UpdateHierarchyLevel(Hierarchy::DataBlockArray& blocks)
{
  for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < blocks.GetCount(); ++uiBlockIndex)
  {
    WorldData::Hierarchy::DataBlock& block = blocks[uiBlockIndex];
    ezGameObject::TransformationData* pCurrentData = block.m_pData;
    ezGameObject::TransformationData* pEndData = block.m_pData + block.m_uiCount;

    while (pCurrentData < pEndData)
    {
      UPDATER::Update(pCurrentData);
      ++pCurrentData;
    }
  }
}

}
