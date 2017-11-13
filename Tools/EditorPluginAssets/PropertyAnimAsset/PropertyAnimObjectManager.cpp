#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

ezPropertyAnimObjectManager::ezPropertyAnimObjectManager()
{
}

ezPropertyAnimObjectManager::~ezPropertyAnimObjectManager()
{
}

ezStatus ezPropertyAnimObjectManager::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return ezStatus(EZ_SUCCESS);

  if (IsTemporary(pParent, szParentProperty))
    return ezStatus("The structure of the context cannot be animated.");
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPropertyAnimObjectManager::InternalCanRemove(const ezDocumentObject* pObject) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return ezStatus(EZ_SUCCESS);

  if (IsTemporary(pObject))
    return ezStatus("The structure of the context cannot be animated.");
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPropertyAnimObjectManager::InternalCanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return ezStatus(EZ_SUCCESS);

  if (IsTemporary(pObject))
    return ezStatus("The structure of the context cannot be animated.");
  return ezStatus(EZ_SUCCESS);
}

bool ezPropertyAnimObjectManager::IsTemporary(const ezDocumentObject* pObject) const
{
  while (pObject->GetParent() != GetRootObject())
  {
    pObject = pObject->GetParent();
  }
  return ezStringUtils::IsEqual(pObject->GetParentProperty(), "TempObjects");
}

bool ezPropertyAnimObjectManager::IsTemporary(const ezDocumentObject* pParent, const char* szParentProperty) const
{
  if (pParent == nullptr || pParent == GetRootObject())
  {
    return ezStringUtils::IsEqual(szParentProperty, "TempObjects");
  }
  return IsTemporary(pParent);
}

