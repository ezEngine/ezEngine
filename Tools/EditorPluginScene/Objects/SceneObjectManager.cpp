#include <PCH.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>
#include <GameEngine/Components/PrefabReferenceComponent.h>

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
  if (pParent == nullptr)
  {
    bool bIsDerived = pRtti->IsDerivedFrom<ezGameObject>();
    if (!bIsDerived)
    {
      return ezStatus("Only ezGameObject can be added to the root of the world!");
    }
  }
  else
  {
    // only prevent adding game objects (as children) to objects that already have a prefab component
    // do allow to attach components to objects with prefab components
    if (pRtti->IsDerivedFrom<ezGameObject>())
    {
      if (pParent->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      {
        auto children = pParent->GetChildren();
        for (auto pChild : children)
        {
          if (pChild->GetType()->IsDerivedFrom<ezPrefabReferenceComponent>())
            return ezStatus("Cannot add objects to a prefab node.");
        }
      }
    }

    if (pRtti->IsDerivedFrom<ezPrefabReferenceComponent>())
    {
      if (!pParent->GetChildren().IsEmpty())
        return ezStatus("Prefab components can only be added to empty nodes.");
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneObjectManager::InternalCanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const
{
  if (pNewParent != nullptr)
  {
    if (pNewParent->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    {
      auto children = pNewParent->GetChildren();
      for (auto pChild : children)
      {
        if (pChild->GetType()->IsDerivedFrom<ezPrefabReferenceComponent>())
          return ezStatus("Cannot move objects into a prefab node.");
      }
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

