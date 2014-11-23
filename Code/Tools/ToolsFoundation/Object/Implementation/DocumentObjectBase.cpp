#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentObjectBaseProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezIReflectedTypeAccessor& ezDocumentObjectBase::GetTypeAccessor()
{
  const ezDocumentObjectBase* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

ezIReflectedTypeAccessor& ezDocumentObjectBase::GetEditorTypeAccessor()
{
  const ezDocumentObjectBase* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetEditorTypeAccessor());
}

ezUInt32 ezDocumentObjectBase::GetChildIndex(ezDocumentObjectBase* pChild) const
{
  return m_Children.IndexOf(pChild);
}

