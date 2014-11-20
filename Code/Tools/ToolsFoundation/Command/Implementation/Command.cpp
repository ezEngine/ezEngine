#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Command/Command.h>

ezCommandBase::ezCommandBase() : m_sDescription(), m_bUndoable(true), m_pDocument(nullptr)
{
}