#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/Command.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommand, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCommand::ezCommand() : m_sDescription(), m_bUndoable(true), m_pDocument(nullptr)
{
}