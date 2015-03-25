
EZ_FORCE_INLINE bool ezRttiTraverser::IsValid(const ezRTTI* pType)
{
  return pType != nullptr;
}

EZ_FORCE_INLINE const ezRTTI* ezRttiTraverser::GetParentType(const ezRTTI* pType)
{
  EZ_ASSERT_DEBUG(pType != nullptr, "Argument to GetParentType must not be nullptr!");
  return pType->GetParentType();
}


template <typename Object>
ezRttiMappedObjectFactory<Object>::ezRttiMappedObjectFactory() : ezRttiMappedObjectFactoryBase<const ezRTTI*, Object, ezRttiTraverser>()
{
}


EZ_FORCE_INLINE bool ezReflectedTypeHandleTraverser::IsValid(ezReflectedTypeHandle hType)
{
  return !hType.IsInvalidated();
}

EZ_FORCE_INLINE ezReflectedTypeHandle ezReflectedTypeHandleTraverser::GetParentType(ezReflectedTypeHandle hType)
{
  EZ_ASSERT_DEBUG(!hType.IsInvalidated(), "Argument to GetParentType must not be invalid!");
  return hType.GetType()->GetParentTypeHandle();
}


template <typename Object>
ezReflectedTypeMappedObjectFactory<Object>::ezReflectedTypeMappedObjectFactory() : ezRttiMappedObjectFactoryBase<ezReflectedTypeHandle, Object, ezReflectedTypeHandleTraverser>()
{
}
