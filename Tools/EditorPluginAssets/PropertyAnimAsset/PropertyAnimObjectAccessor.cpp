#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

ezPropertyAnimObjectAccessor::ezPropertyAnimObjectAccessor(ezPropertyAnimAssetDocument* pDoc, ezCommandHistory* pHistory)
  : ezObjectCommandAccessor(pHistory)
  , m_ObjAccessor(pHistory)
  , m_pDocument(pDoc)
  , m_pObjectManager(static_cast<ezPropertyAnimObjectManager*>(pDoc->GetObjectManager()))
{
}

ezStatus ezPropertyAnimObjectAccessor::GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index /*= ezVariant()*/)
{
  if (IsTemporary(pObject))
  {

    return ezObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
  }
  else
  {
    return ezObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  if (IsTemporary(pObject))
  {
   /* const ezRTTI* pObjType = ezGetStaticRTTI<ezGameObject>();
    const ezAbstractProperty* pName = pObjType->FindPropertyByName("Name");
    ezStringBuilder sObjectSearchSequence;
    ezStringBuilder sComponentType;
    ezStringBuilder sPropertyPath = pProp->GetPropertyName();
    const ezDocumentObject* pObj = pObject;
    while (pObj != m_pObjectManager->GetRootObject())
    {
      if (pObj->GetType() == ezGetStaticRTTI<ezGameObject>())
      {
        ezString sName = m_ObjAccessor.Get<ezString>(pObj, pName);
        if (!sName.IsEmpty())
        {
          if (!sObjectSearchSequence.IsEmpty())
            sObjectSearchSequence.Prepend("/");
          sObjectSearchSequence.Prepend(sName);
        }
      }
      else if (pObj->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezComponent>()))
      {
        sComponentType = pObj->GetType()->GetTypeName();
      }
      else
      {
        if (!sPropertyPath.IsEmpty())
          sPropertyPath.Prepend("/");
        sPropertyPath.Prepend(pObj->GetParentPropertyType()->GetPropertyName());
      }
      pObj = pObj->GetParent();
    }*/

    return ezObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
  else
  {
    return ezObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

ezStatus ezPropertyAnimObjectAccessor::AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  if (IsTemporary(pParent, pParentProp))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

ezStatus ezPropertyAnimObjectAccessor::RemoveObject(const ezDocumentObject* pObject)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::RemoveObject(pObject);
  }
}

ezStatus ezPropertyAnimObjectAccessor::MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::MoveObject(pObject, pNewParent, pParentProp, index);
  }
}

bool ezPropertyAnimObjectAccessor::IsTemporary(const ezDocumentObject* pObject) const
{
  return m_pObjectManager->IsTemporary(pObject);
}

bool ezPropertyAnimObjectAccessor::IsTemporary(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp) const
{
  return m_pObjectManager->IsTemporary(pParent, pParentProp->GetPropertyName());
}
