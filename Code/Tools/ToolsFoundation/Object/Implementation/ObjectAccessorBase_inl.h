#include <Foundation/Logging/Log.h>

template<typename T>
T ezObjectAccessorBase::Get(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  ezVariant value;
  ezStatus res = GetValue(pObject, pProp, value, index);
  if (res.m_Result.Failed())
    ezLog::ErrorPrintf("GetValue failed: %s", res.m_sMessage.GetData());
  return value.ConvertTo<T>();
}

ezInt32 ezObjectAccessorBase::GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  ezInt32 iCount = 0;
  ezStatus res = GetCount(pObject, pProp, iCount);
  if (res.m_Result.Failed())
    ezLog::ErrorPrintf("GetCount failed: %s", res.m_sMessage.GetData());
  return iCount;
}

