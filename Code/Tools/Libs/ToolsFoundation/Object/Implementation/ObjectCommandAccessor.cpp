#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

ezObjectCommandAccessor::ezObjectCommandAccessor(ezCommandHistory* pHistory)
  : ezObjectDirectAccessor(const_cast<ezDocumentObjectManager*>(pHistory->GetDocument()->GetObjectManager()))
  , m_pHistory(pHistory)
{
}

void ezObjectCommandAccessor::StartTransaction(ezStringView sDisplayString)
{
  m_pHistory->StartTransaction(sDisplayString);
}

void ezObjectCommandAccessor::CancelTransaction()
{
  m_pHistory->CancelTransaction();
}

void ezObjectCommandAccessor::FinishTransaction()
{
  m_pHistory->FinishTransaction();
}

void ezObjectCommandAccessor::BeginTemporaryCommands(ezStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pHistory->BeginTemporaryCommands(sDisplayString, bFireEventsWhenUndoingTempCommands);
}

void ezObjectCommandAccessor::CancelTemporaryCommands()
{
  m_pHistory->CancelTemporaryCommands();
}

void ezObjectCommandAccessor::FinishTemporaryCommands()
{
  m_pHistory->FinishTemporaryCommands();
}

ezStatus ezObjectCommandAccessor::SetValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_NewValue = newValue;
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

ezStatus ezObjectCommandAccessor::InsertValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  ezInsertObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_NewValue = newValue;
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

ezStatus ezObjectCommandAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  ezRemoveObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

ezStatus ezObjectCommandAccessor::MoveValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  ezMoveObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_OldIndex = oldIndex;
  cmd.m_NewIndex = newIndex;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

ezStatus ezObjectCommandAccessor::AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  ezAddObjectCommand cmd;
  cmd.m_Parent = pParent ? pParent->GetGuid() : ezUuid();
  cmd.m_Index = index;
  cmd.m_pType = pType;
  cmd.m_NewObjectGuid = inout_objectGuid;
  cmd.m_sParentProperty = pParentProp ? pParentProp->GetPropertyName() : "Children";
  ezStatus res = m_pHistory->AddCommand(cmd);
  if (res.m_Result.Succeeded())
    inout_objectGuid = cmd.m_NewObjectGuid;
  return res;
}

ezStatus ezObjectCommandAccessor::RemoveObject(const ezDocumentObject* pObject)
{
  ezRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  return m_pHistory->AddCommand(cmd);
}

ezStatus ezObjectCommandAccessor::MoveObject(
  const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index)
{
  ezMoveObjectCommand cmd;
  cmd.m_NewParent = pNewParent ? pNewParent->GetGuid() : ezUuid();
  cmd.m_Object = pObject->GetGuid();
  cmd.m_Index = index;
  cmd.m_sParentProperty = pParentProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}
