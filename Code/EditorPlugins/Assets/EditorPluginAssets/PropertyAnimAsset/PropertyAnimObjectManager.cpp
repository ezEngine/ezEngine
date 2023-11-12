#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

ezPropertyAnimObjectManager::ezPropertyAnimObjectManager() = default;

ezPropertyAnimObjectManager::~ezPropertyAnimObjectManager() = default;

ezStatus ezPropertyAnimObjectManager::InternalCanAdd(
  const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return ezStatus(EZ_SUCCESS);

  if (IsTemporary(pParent, sParentProperty))
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

ezStatus ezPropertyAnimObjectManager::InternalCanMove(
  const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, ezStringView sParentProperty, const ezVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return ezStatus(EZ_SUCCESS);

  if (IsTemporary(pObject))
    return ezStatus("The structure of the context cannot be animated.");
  return ezStatus(EZ_SUCCESS);
}
