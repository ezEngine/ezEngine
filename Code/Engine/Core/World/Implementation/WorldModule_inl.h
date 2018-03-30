
EZ_ALWAYS_INLINE ezWorld* ezWorldModule::GetWorld()
{
  return m_pWorld;
}

EZ_ALWAYS_INLINE const ezWorld* ezWorldModule::GetWorld() const
{
  return m_pWorld;
}

//////////////////////////////////////////////////////////////////////////

template <typename ModuleType, typename RTTIType>
ezUInt16 ezWorldModuleFactory::RegisterWorldModule()
{
  struct Helper
  {
    static ezWorldModule* Create(ezAllocatorBase* pAllocator, ezWorld* pWorld)
    {
      return EZ_NEW(pAllocator, ModuleType, pWorld);
    }
  };

  const ezRTTI* pRtti = ezGetStaticRTTI<RTTIType>();
  return RegisterWorldModule(pRtti, &Helper::Create);
}

