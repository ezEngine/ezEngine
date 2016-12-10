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


ezStatus ezObjectAccessorBase::GetValue(const ezDocumentObject* pObject, const char* szProp, ezVariant& out_value, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetValue(pObject, pProp, out_value, index);
}


ezStatus ezObjectAccessorBase::SetValue(const ezDocumentObject* pObject, const char* szProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return SetValue(pObject, pProp, newValue, index);
}


ezStatus ezObjectAccessorBase::InsertValue(const ezDocumentObject* pObject, const char* szProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return InsertValue(pObject, pProp, newValue, index);
}


ezStatus ezObjectAccessorBase::RemoveValue(const ezDocumentObject* pObject, const char* szProp, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return RemoveValue(pObject, pProp, index);
}


ezStatus ezObjectAccessorBase::MoveValue(const ezDocumentObject* pObject, const char* szProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return MoveValue(pObject, pProp, oldIndex, newIndex);
}


ezStatus ezObjectAccessorBase::GetCount(const ezDocumentObject* pObject, const char* szProp, ezInt32& out_iCount)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetCount(pObject, pProp, out_iCount);
}


ezStatus ezObjectAccessorBase::AddObject(const ezDocumentObject* pParent, const char* szParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  const ezAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(szParentProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szParentProp, pParent->GetType()->GetTypeName()));
  return AddObject(pParent, pProp, index, pType, inout_objectGuid);
}

ezStatus ezObjectAccessorBase::MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProp, const ezVariant& index)
{
  const ezAbstractProperty* pProp = pNewParent->GetType()->FindPropertyByName(szParentProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szParentProp, pNewParent->GetType()->GetTypeName()));
  return MoveObject(pObject, pNewParent, pProp, index);
}


ezStatus ezObjectAccessorBase::GetKeys(const ezDocumentObject* pObject, const char* szProp, ezHybridArray<ezVariant, 16>& out_keys)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetKeys(pObject, pProp, out_keys);
}


ezStatus ezObjectAccessorBase::GetValues(const ezDocumentObject* pObject, const char* szProp, ezHybridArray<ezVariant, 16>& out_values)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetValues(pObject, pProp, out_values);
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
