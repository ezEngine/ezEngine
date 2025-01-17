#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void ezObjectAccessorBase::StartTransaction(ezStringView sDisplayString) {}


void ezObjectAccessorBase::CancelTransaction() {}


void ezObjectAccessorBase::FinishTransaction() {}


void ezObjectAccessorBase::BeginTemporaryCommands(ezStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/) {}


void ezObjectAccessorBase::CancelTemporaryCommands() {}


void ezObjectAccessorBase::FinishTemporaryCommands() {}


ezStatus ezObjectAccessorBase::GetValueByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant& out_value, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValue(pObject, pProp, out_value, index);
}


ezStatus ezObjectAccessorBase::SetValueByName(const ezDocumentObject* pObject, ezStringView sProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return SetValue(pObject, pProp, newValue, index);
}


ezStatus ezObjectAccessorBase::InsertValueByName(const ezDocumentObject* pObject, ezStringView sProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return InsertValue(pObject, pProp, newValue, index);
}


ezStatus ezObjectAccessorBase::RemoveValueByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant index /*= ezVariant()*/)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return RemoveValue(pObject, pProp, index);
}


ezStatus ezObjectAccessorBase::MoveValueByName(const ezDocumentObject* pObject, ezStringView sProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return MoveValue(pObject, pProp, oldIndex, newIndex);
}


ezStatus ezObjectAccessorBase::GetCountByName(const ezDocumentObject* pObject, ezStringView sProp, ezInt32& out_iCount)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetCount(pObject, pProp, out_iCount);
}


ezStatus ezObjectAccessorBase::AddObjectByName(const ezDocumentObject* pParent, ezStringView sParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  const ezAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pParent->GetType()->GetTypeName()));
  return AddObject(pParent, pProp, index, pType, inout_objectGuid);
}

ezStatus ezObjectAccessorBase::MoveObjectByName(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, ezStringView sParentProp, const ezVariant& index)
{
  const ezAbstractProperty* pProp = pNewParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pNewParent->GetType()->GetTypeName()));
  return MoveObject(pObject, pNewParent, pProp, index);
}


ezStatus ezObjectAccessorBase::GetKeysByName(const ezDocumentObject* pObject, ezStringView sProp, ezDynamicArray<ezVariant>& out_keys)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetKeys(pObject, pProp, out_keys);
}


ezStatus ezObjectAccessorBase::GetValuesByName(const ezDocumentObject* pObject, ezStringView sProp, ezDynamicArray<ezVariant>& out_values)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValues(pObject, pProp, out_values);
}

const ezDocumentObject* ezObjectAccessorBase::GetChildObjectByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant index)
{
  ezVariant value;
  if (GetValueByName(pObject, sProp, value, index).Succeeded() && value.IsA<ezUuid>())
  {
    return GetObject(value.Get<ezUuid>());
  }
  return nullptr;
}

ezStatus ezObjectAccessorBase::ClearByName(const ezDocumentObject* pObject, ezStringView sProp)
{
  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return ezStatus(ezFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));

  ezHybridArray<ezVariant, 8> keys;
  ezStatus res = GetKeys(pObject, pProp, keys);
  if (res.Failed())
    return res;

  for (ezInt32 i = keys.GetCount() - 1; i >= 0; --i)
  {
    res = RemoveValue(pObject, pProp, keys[i]);
    if (res.Failed())
      return res;
  }
  return ezStatus(EZ_SUCCESS);
}

ezObjectAccessorBase::ezObjectAccessorBase(const ezDocumentObjectManager* pManager)
  : m_pConstManager(pManager)
{
}

ezObjectAccessorBase::~ezObjectAccessorBase() = default;

const ezDocumentObjectManager* ezObjectAccessorBase::GetObjectManager() const
{
  return m_pConstManager;
}

void ezObjectAccessorBase::FireDocumentObjectStructureEvent(const ezDocumentObjectStructureEvent& e)
{
  m_pConstManager->m_StructureEvents.Broadcast(e);
}

void ezObjectAccessorBase::FireDocumentObjectPropertyEvent(const ezDocumentObjectPropertyEvent& e)
{
  m_pConstManager->m_PropertyEvents.Broadcast(e);
}
