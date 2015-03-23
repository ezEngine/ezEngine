#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezActionMapDescriptor, ezNoBase, 0, ezRTTINoAllocator);
//  EZ_BEGIN_PROPERTIES
//  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();


////////////////////////////////////////////////////////////////////////
// ezActionObjectManager public functions
////////////////////////////////////////////////////////////////////////

ezActionObjectManager::ezActionObjectManager() : ezDocumentObjectManagerBase()
{
}

void ezActionObjectManager::GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const
{
  Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezActionMapDescriptor>()->GetTypeName()));
}


////////////////////////////////////////////////////////////////////////
// ezActionObjectManager private functions
////////////////////////////////////////////////////////////////////////

ezDocumentObjectBase* ezActionObjectManager::InternalCreateObject(ezReflectedTypeHandle hType)
{
  return new ezActionMap::ObjectType;
}

void ezActionObjectManager::InternalDestroyObject(ezDocumentObjectBase* pObject)
{
  delete pObject;
}

bool ezActionObjectManager::InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const
{
  return true;
}

bool ezActionObjectManager::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezActionObjectManager::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  return true;
}



////////////////////////////////////////////////////////////////////////
// ezActionMap public functions
////////////////////////////////////////////////////////////////////////

ezActionMap::ezActionMap() : ezDocumentObjectTree()
{
  m_Manager.SetObjectTree(this);

  ezReflectedTypeDescriptor desc;
  ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(ezGetStaticRTTI<ezActionMapDescriptor>(), desc);
  m_hDesc = ezReflectedTypeManager::RegisterType(desc);
}

ezActionMap::~ezActionMap()
{
}

ezUuid ezActionMap::MapAction(const ezActionMapDescriptor& desc)
{
  ezUuid parent;
  if (!FindObjectByPath(desc.m_sPath, parent))
  {
    return ezUuid();
  }
  

  ezDocumentObjectBase* pParent = GetObject(parent);
  const ezActionMapDescriptor* pDesc = GetDescriptor(pParent);
  if (pDesc->m_hAction.GetDescriptor()->m_Type == ActionType::Action)
  {
    ezLog::Error("Can't map descriptor '%s' as its parent is an action itself and thus can't have any children.", desc.m_hAction.GetDescriptor()->m_sActionName.GetData());
    return ezUuid();
  }

  const ezDocumentObjectBase* pChild = GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName);
  if (pChild != nullptr)
  {
    ezLog::Error("Can't map descriptor as its name is already present: %s", desc.m_hAction.GetDescriptor()->m_sActionName.GetData());
    return ezUuid();
  }

  ObjectType* pCastObject = static_cast<ObjectType*>(m_Manager.CreateObject(m_hDesc));
  pCastObject->m_MemberProperties = desc;

  ezInt32 iIndex = 0;
  for(ezUInt32 i = 0; i < pParent->GetChildren().GetCount(); ++i)
  {
    const ezDocumentObjectBase* pChild = pParent->GetChildren()[i];
    const ezActionMapDescriptor* pDesc = GetDescriptor(pChild);

    if (desc.m_fOrder < pDesc->m_fOrder)
    {
      iIndex = i;
      break;
    }
  }

  AddObject(pCastObject, pParent, iIndex);

  return pCastObject->GetGuid();
}

ezResult ezActionMap::UnmapAction(const ezUuid& guid)
{
  ezDocumentObjectBase* pObject = GetObject(guid);
  if (!pObject)
    return EZ_FAILURE;

  RemoveObject(pObject);
  m_Manager.DestroyObject(pObject);
  return EZ_SUCCESS;
}

bool ezActionMap::FindObjectByPath(const ezStringView& sPath, ezUuid& out_guid) const
{
  out_guid = ezUuid();
  if (sPath.IsEmpty())
    return true;

  ezStringBuilder sPathBuilder(sPath);
  ezHybridArray<ezStringView, 8> parts;
  sPathBuilder.Split(false, parts, "/");

  const ezDocumentObjectBase* pParent = GetRootObject();
  for(const ezStringView& name : parts)
  {
    pParent = GetChildByName(pParent, name);
    if (pParent == nullptr)
      return false;
  }

  out_guid = pParent->GetGuid(); 
  return true;
}

const ezActionMapDescriptor* ezActionMap::GetDescriptor(const ezUuid& guid) const
{
  return GetDescriptor(GetObject(guid));
}

const ezActionMapDescriptor* ezActionMap::GetDescriptor(const ezDocumentObjectBase* pObject) const
{
  if (pObject == nullptr)
    return nullptr;

  const ObjectType* pCastObject = static_cast<const ObjectType*>(pObject);
  return &pCastObject->m_MemberProperties;
}


////////////////////////////////////////////////////////////////////////
// ezActionMap private functions
////////////////////////////////////////////////////////////////////////

const ezDocumentObjectBase* ezActionMap::GetChildByName(const ezDocumentObjectBase* pObject, const ezStringView& sName) const
{
  for(const ezDocumentObjectBase* pChild : pObject->GetChildren())
  {
    const ezActionMapDescriptor* pDesc = GetDescriptor(pChild);
    if (sName.IsEqual_NoCase(pDesc->m_hAction.GetDescriptor()->m_sActionName))
    {
      return pChild;
    }
  }
  return nullptr;
}

