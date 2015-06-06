#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeSubObjectAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include<Foundation/Reflection/Reflection.h>

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeSubObjectAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeSubObjectAccessor::ezReflectedTypeSubObjectAccessor(const ezRTTI* pRtti)
  : ezIReflectedTypeAccessor(pRtti)
{

}

const ezVariant ezReflectedTypeSubObjectAccessor::GetValue(const ezPropertyPath& path) const
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->GetValue(pathCopy);
}

bool ezReflectedTypeSubObjectAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value)
{
  ezPropertyPath pathCopy = m_SubPath;
  pathCopy.PushBackRange(path);

  return m_pAcc->SetValue(pathCopy, value);
}

void ezReflectedTypeSubObjectAccessor::SetSubAccessor(ezIReflectedTypeAccessor* pAcc, const ezPropertyPath& subPath)
{
  m_SubPath = subPath;
  m_pAcc = pAcc;
}
