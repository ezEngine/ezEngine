#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeSubObjectAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include<Foundation/Reflection/Reflection.h>

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeSubObjectAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeSubObjectAccessor::ezReflectedTypeSubObjectAccessor(const ezRTTI* pRtti, ezDocumentObjectBase* pOwner)
  : ezIReflectedTypeAccessor(pRtti, pOwner)
{

}

void ezReflectedTypeSubObjectAccessor::SetSubAccessor(ezIReflectedTypeAccessor* pAcc, const ezPropertyPath& subPath)
{
  m_SubPath = subPath;
  m_pAcc = pAcc;
}

const ezVariant ezReflectedTypeSubObjectAccessor::GetValue(const ezPropertyPath& path, ezVariant index) const
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->GetValue(pathCopy, index);
}

bool ezReflectedTypeSubObjectAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value, ezVariant index)
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->SetValue(pathCopy, value, index);
}

ezInt32 ezReflectedTypeSubObjectAccessor::GetCount(const ezPropertyPath& path) const
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->GetCount(pathCopy);
}

bool ezReflectedTypeSubObjectAccessor::GetKeys(const ezPropertyPath& path, ezHybridArray<ezVariant, 16>& out_keys) const
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->GetKeys(pathCopy, out_keys);
}

bool ezReflectedTypeSubObjectAccessor::InsertValue(const ezPropertyPath& path, ezVariant index, const ezVariant& value)
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->InsertValue(pathCopy, index, value);
}

bool ezReflectedTypeSubObjectAccessor::RemoveValue(const ezPropertyPath& path, ezVariant index)
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->RemoveValue(pathCopy, index);
}

bool ezReflectedTypeSubObjectAccessor::MoveValue(const ezPropertyPath& path, ezVariant oldIndex, ezVariant newIndex)
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->MoveValue(pathCopy, oldIndex, newIndex);
}

ezVariant ezReflectedTypeSubObjectAccessor::GetPropertyChildIndex(const ezPropertyPath& path, const ezVariant& value) const
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->GetPropertyChildIndex(pathCopy, value);
}


