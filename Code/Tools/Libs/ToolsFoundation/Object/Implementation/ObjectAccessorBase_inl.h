#include <Foundation/Logging/Log.h>

template <typename T>
T ezObjectAccessorBase::Get(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  ezVariant value;
  ezStatus res = GetValue(pObject, pProp, value, index);
  if (res.m_Result.Failed())
    ezLog::Error("GetValue failed: {0}", res.m_sMessage);
  return value.ConvertTo<T>();
}

template <typename T>
T ezObjectAccessorBase::GetByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant index /*= ezVariant()*/)
{
  ezVariant value;
  ezStatus res = GetValueByName(pObject, sProp, value, index);
  if (res.m_Result.Failed())
    ezLog::Error("GetValue failed: {0}", res.m_sMessage);
  return value.ConvertTo<T>();
}

inline ezInt32 ezObjectAccessorBase::GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  ezInt32 iCount = 0;
  ezStatus res = GetCount(pObject, pProp, iCount);
  if (res.m_Result.Failed())
    ezLog::Error("GetCount failed: {0}", res.m_sMessage);
  return iCount;
}

inline ezInt32 ezObjectAccessorBase::GetCountByName(const ezDocumentObject* pObject, ezStringView sProp)
{
  ezInt32 iCount = 0;
  ezStatus res = GetCountByName(pObject, sProp, iCount);
  if (res.m_Result.Failed())
    ezLog::Error("GetCount failed: {0}", res.m_sMessage);
  return iCount;
}
