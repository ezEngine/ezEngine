#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void ezObjectAccessorBase::StartTransaction(const char* szDisplayString)
{
}


void ezObjectAccessorBase::CancelTransaction()
{
}


void ezObjectAccessorBase::FinishTransaction()
{
}


void ezObjectAccessorBase::BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{

}


void ezObjectAccessorBase::CancelTemporaryCommands()
{

}


void ezObjectAccessorBase::FinishTemporaryCommands()
{

}

ezObjectAccessorBase::ezObjectAccessorBase(ezDocumentObjectManager* pManager)
  : m_pManager(pManager)
{
}

void ezObjectAccessorBase::FireDocumentObjectStructureEvent(const ezDocumentObjectStructureEvent& e)
{
  m_pManager->m_StructureEvents.Broadcast(e);
}

void ezObjectAccessorBase::FireDocumentObjectPropertyEvent(const ezDocumentObjectPropertyEvent& e)
{
  m_pManager->m_PropertyEvents.Broadcast(e);
}
