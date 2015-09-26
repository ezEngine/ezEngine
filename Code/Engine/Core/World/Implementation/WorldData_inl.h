
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
  EZ_FORCE_INLINE void WorldData::UpdateGlobalTransform(ezGameObject::TransformationData* pData, float fInvDeltaSeconds)
  {
    const ezVec3 vOldWorldPos = pData->m_globalTransform.m_vPosition;
    pData->UpdateGlobalTransform();
    pData->UpdateGlobalBounds();
    pData->m_velocity = ((pData->m_globalTransform.m_vPosition - vOldWorldPos) * fInvDeltaSeconds).GetAsDirectionVec4();
  }

  // static
  EZ_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParent(ezGameObject::TransformationData* pData, float fInvDeltaSeconds)
  {
    const ezVec3 vOldWorldPos = pData->m_globalTransform.m_vPosition;
    pData->UpdateGlobalTransformWithParent();
    pData->UpdateGlobalBounds();
    pData->m_velocity = ((pData->m_globalTransform.m_vPosition - vOldWorldPos) * fInvDeltaSeconds).GetAsDirectionVec4();
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  EZ_FORCE_INLINE WorldData::ReadMarker::ReadMarker(const WorldData& data) : m_Data(data)
  {
  }

  EZ_FORCE_INLINE void WorldData::ReadMarker::Acquire()
  {
    EZ_ASSERT_DEV(m_Data.m_WriteThreadID == (ezThreadID)0 || m_Data.m_WriteThreadID == ezThreadUtils::GetCurrentThreadID(),
      "World '%s' cannot be marked for reading because it is already marked for writing by another thread.", m_Data.m_sName.GetData());
    m_Data.m_iReadCounter.Increment();
  }

  EZ_FORCE_INLINE void WorldData::ReadMarker::Release()
  {
    m_Data.m_iReadCounter.Decrement();
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  EZ_FORCE_INLINE WorldData::WriteMarker::WriteMarker(WorldData& data) : m_Data(data)
  {
  }

  EZ_FORCE_INLINE void WorldData::WriteMarker::Acquire()
  {
    // already locked by this thread?
    if (m_Data.m_WriteThreadID != ezThreadUtils::GetCurrentThreadID())
    {
      EZ_ASSERT_DEV(m_Data.m_iReadCounter == 0, "World '%s' cannot be marked for writing because it is already marked for reading.", m_Data.m_sName.GetData());
      EZ_ASSERT_DEV(m_Data.m_WriteThreadID == (ezThreadID)0, "World '%s' cannot be marked for writing because it is already marked for writing by another thread.", m_Data.m_sName.GetData());

      m_Data.m_WriteThreadID = ezThreadUtils::GetCurrentThreadID();
      m_Data.m_iReadCounter.Increment(); // allow reading as well
    }

    m_Data.m_iWriteCounter++;
  }

  EZ_FORCE_INLINE void WorldData::WriteMarker::Release()
  {
    m_Data.m_iWriteCounter--;

    if (m_Data.m_iWriteCounter == 0)
    {
      m_Data.m_iReadCounter.Decrement();
      m_Data.m_WriteThreadID = (ezThreadID)0;      
    }
  }

}
