#include <PCH.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>

ezSceneObjectManager::ezSceneObjectManager() : ezDocumentObjectManager()
{
}

void ezSceneObjectManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  Types.PushBack(ezRTTI::FindTypeByName(ezGetStaticRTTI<ezGameObject>()->GetTypeName()));

  const ezRTTI* pComponentType = ezRTTI::FindTypeByName(ezGetStaticRTTI<ezComponent>()->GetTypeName());

  for (auto it = ezRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pComponentType) && !it->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

ezStatus ezSceneObjectManager::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const
{
  const ezRTTI* pGameObjectType = ezRTTI::FindTypeByName(ezGetStaticRTTI<ezGameObject>()->GetTypeName());

  if (pParent == nullptr)
  {
    bool bIsDerived = pRtti->IsDerivedFrom(pGameObjectType);
    if (!bIsDerived)
    {
      return ezStatus("Only ezGameObject can be added to the root of the world!");
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneObjectManager::InternalCanSelect(const ezDocumentObject* pObject) const
{
  if (pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
  {
    return ezStatus(ezFmt("Object of type '{0}' is not a 'ezGameObject' and can't be selected.", pObject->GetTypeAccessor().GetType()->GetTypeName()));
  }
  return ezStatus(EZ_SUCCESS);
}
