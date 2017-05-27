
namespace ezInternal
{
  //static
  EZ_ALWAYS_INLINE WorldData::HierarchyType::Enum WorldData::GetHierarchyType(bool bIsDynamic)
  {
    return bIsDynamic ? HierarchyType::Dynamic : HierarchyType::Static;
  }

  // static
  template <typename VISITOR>
  EZ_FORCE_INLINE ezVisitorExecution::Enum WorldData::TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData /* = nullptr*/)
  {
    for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < blocks.GetCount(); ++uiBlockIndex)
    {
      WorldData::Hierarchy::DataBlock& block = blocks[uiBlockIndex];
      ezGameObject::TransformationData* pCurrentData = block.m_pData;
      ezGameObject::TransformationData* pEndData = block.m_pData + block.m_uiCount;

      while (pCurrentData < pEndData)
      {
        if (VISITOR::Visit(pCurrentData, pUserData) == ezVisitorExecution::Stop)
          return ezVisitorExecution::Stop;

        ++pCurrentData;
      }
    }

    return ezVisitorExecution::Continue;
  }

  // static
  EZ_FORCE_INLINE void WorldData::UpdateGlobalTransform(ezGameObject::TransformationData* pData, const ezSimdFloat& fInvDeltaSeconds)
  {
    pData->UpdateGlobalTransform();
    pData->UpdateGlobalBounds();
    pData->UpdateVelocity(fInvDeltaSeconds);
  }

  // static
  EZ_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParent(ezGameObject::TransformationData* pData, const ezSimdFloat& fInvDeltaSeconds)
  {
    pData->UpdateGlobalTransformWithParent();
    pData->UpdateGlobalBounds();
    pData->UpdateVelocity(fInvDeltaSeconds);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  EZ_FORCE_INLINE void WorldData::RegisteredUpdateFunction::FillFromDesc(const ezWorldModule::UpdateFunctionDesc& desc)
  {
    m_Function = desc.m_Function;
    m_sFunctionName = desc.m_sFunctionName;
    m_fPriority = desc.m_fPriority;
    m_uiGranularity = desc.m_uiGranularity;
    m_bOnlyUpdateWhenSimulating = desc.m_bOnlyUpdateWhenSimulating;
  }

  EZ_FORCE_INLINE bool WorldData::RegisteredUpdateFunction::operator<(const RegisteredUpdateFunction& other) const
  {
    // higher priority comes first
    if (m_fPriority != other.m_fPriority)
      return m_fPriority > other.m_fPriority;

    // sort by function name to ensure determinism
    ezInt32 iNameComp = ezStringUtils::Compare(m_sFunctionName, other.m_sFunctionName);
    EZ_ASSERT_DEV(iNameComp != 0, "An update function with the same name and same priority is already registered. This breaks determinism.");
    return iNameComp < 0;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  EZ_ALWAYS_INLINE WorldData::ReadMarker::ReadMarker(const WorldData& data) : m_Data(data)
  {
  }

  EZ_FORCE_INLINE void WorldData::ReadMarker::Acquire()
  {
    EZ_ASSERT_DEV(m_Data.m_WriteThreadID == (ezThreadID)0 || m_Data.m_WriteThreadID == ezThreadUtils::GetCurrentThreadID(),
      "World '{0}' cannot be marked for reading because it is already marked for writing by another thread.", m_Data.m_sName.GetData());
    m_Data.m_iReadCounter.Increment();
  }

  EZ_ALWAYS_INLINE void WorldData::ReadMarker::Release()
  {
    m_Data.m_iReadCounter.Decrement();
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  EZ_ALWAYS_INLINE WorldData::WriteMarker::WriteMarker(WorldData& data) : m_Data(data)
  {
  }

  EZ_FORCE_INLINE void WorldData::WriteMarker::Acquire()
  {
    // already locked by this thread?
    if (m_Data.m_WriteThreadID != ezThreadUtils::GetCurrentThreadID())
    {
      EZ_ASSERT_DEV(m_Data.m_iReadCounter == 0, "World '{0}' cannot be marked for writing because it is already marked for reading.", m_Data.m_sName.GetData());
      EZ_ASSERT_DEV(m_Data.m_WriteThreadID == (ezThreadID)0, "World '{0}' cannot be marked for writing because it is already marked for writing by another thread.", m_Data.m_sName.GetData());

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
