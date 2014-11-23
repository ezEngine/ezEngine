#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/Command.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommandBase, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCommandBase::ezCommandBase() : m_sDescription(), m_bUndoable(true), m_pDocument(nullptr)
{
}