#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezObjectDirectAccessor::ezObjectDirectAccessor(ezDocumentObjectManager* pManager)
  : ezObjectAccessorBase(pManager)
{

}

const ezDocumentObject* ezObjectDirectAccessor::GetObject(const ezUuid& object)
{
  return m_pManager->GetObject(object);
}

ezStatus ezObjectDirectAccessor::GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index)
{
  out_value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index);
  if (!out_value.IsValid())
    return ezStatus("GetValue returned an invalid value.");
  else
    return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectDirectAccessor::SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  ezDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  EZ_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().SetValue(pProp->GetPropertyName(), newValue, index);
  return ezStatus(bRes ? EZ_SUCCESS : EZ_FAILURE);
}

ezStatus ezObjectDirectAccessor::InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  ezDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  EZ_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), index, newValue);
  return ezStatus(bRes ? EZ_SUCCESS : EZ_FAILURE);
}

ezStatus ezObjectDirectAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  EZ_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), index);
  return ezStatus(bRes ? EZ_SUCCESS : EZ_FAILURE);
}

ezStatus ezObjectDirectAccessor::MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  ezDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  EZ_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().MoveValue(pProp->GetPropertyName(), oldIndex, newIndex);
  return ezStatus(bRes ? EZ_SUCCESS : EZ_FAILURE);
}

ezStatus ezObjectDirectAccessor::GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount)
{
  out_iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectDirectAccessor::AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  EZ_SUCCEED_OR_RETURN(m_pManager->CanAdd(pType, pParent, pParentProp->GetPropertyName(), index));

  ezDocumentObject* pPar = m_pManager->GetObject(pParent->GetGuid());
  EZ_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  if (!inout_objectGuid.IsValid())
    inout_objectGuid.CreateNewUuid();
  ezDocumentObject* pObj = m_pManager->CreateObject(pType, inout_objectGuid);
  m_pManager->AddObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectDirectAccessor::RemoveObject(const ezDocumentObject* pObject)
{
  EZ_SUCCEED_OR_RETURN(m_pManager->CanRemove(pObject));

  ezDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  EZ_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  m_pManager->RemoveObject(pObj);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectDirectAccessor::MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index)
{
  EZ_SUCCEED_OR_RETURN(m_pManager->CanMove(pObject, pNewParent, pParentProp->GetPropertyName(), index));

  ezDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  EZ_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  ezDocumentObject* pPar = m_pManager->GetObject(pNewParent->GetGuid());
  EZ_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  m_pManager->MoveObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezObjectDirectAccessor::GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_keys)
{
  bool bRes = pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), out_keys);
  return ezStatus(bRes ? EZ_SUCCESS : EZ_FAILURE);
}

ezStatus ezObjectDirectAccessor::GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_values)
{
  bool bRes = pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), out_values);
  return ezStatus(bRes ? EZ_SUCCESS : EZ_FAILURE);
}
