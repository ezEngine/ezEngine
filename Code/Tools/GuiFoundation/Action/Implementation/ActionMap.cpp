#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezActionMapDescriptor, ezNoBase, 0, ezRTTINoAllocator);
//  EZ_BEGIN_PROPERTIES
//  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();


////////////////////////////////////////////////////////////////////////
// ezActionMap public functions
////////////////////////////////////////////////////////////////////////

ezActionMap::ezActionMap() : ezDocumentObjectManager()
{
  ezReflectedTypeDescriptor desc;
  ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(ezGetStaticRTTI<ezActionMapDescriptor>(), desc);
  m_pRtti = ezPhantomRttiManager::RegisterType(desc);
}

ezActionMap::~ezActionMap()
{
}

void ezActionMap::MapAction(ezActionDescriptorHandle hAction, const char* szPath, float fOrder)
{
  ezActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath = szPath;
  d.m_fOrder = fOrder;

  EZ_VERIFY(MapAction(d).IsValid(), "Mapping Failed");
}

ezUuid ezActionMap::MapAction(const ezActionMapDescriptor& desc)
{
  ezUuid ParentGUID;
  if (!FindObjectByPath(desc.m_sPath, ParentGUID))
  {
    return ezUuid();
  }
  

  ezDocumentObjectBase* pParent = GetObject(ParentGUID);
  if (desc.m_sPath.IsEmpty())
  {
    pParent = GetRootObject();
  }
  else
  {
    const ezActionMapDescriptor* pDesc = GetDescriptor(pParent);
    if (pDesc->m_hAction.GetDescriptor()->m_Type == ezActionType::Action)
    {
      ezLog::Error("Can't map descriptor '%s' as its parent is an action itself and thus can't have any children.", desc.m_hAction.GetDescriptor()->m_sActionName.GetData());
      return ezUuid();
    }
  }

  const ezDocumentObjectBase* pChild = GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName);
  if (pChild != nullptr)
  {
    ezLog::Error("Can't map descriptor as its name is already present: %s", desc.m_hAction.GetDescriptor()->m_sActionName.GetData());
    return ezUuid();
  }

  ObjectType* pCastObject = static_cast<ObjectType*>(CreateObject(m_pRtti));
  pCastObject->m_MemberProperties = desc;

  ezInt32 iIndex = 0;
  for(iIndex = 0; iIndex < (ezInt32) pParent->GetChildren().GetCount(); ++iIndex)
  {
    const ezDocumentObjectBase* pChild = pParent->GetChildren()[iIndex];
    const ezActionMapDescriptor* pDesc = GetDescriptor(pChild);

    if (desc.m_fOrder < pDesc->m_fOrder)
      break;
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
  DestroyObject(pObject);
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

const ezDocumentObjectBase* ezActionMap::GetChildByName(const ezDocumentObjectBase* pObject, const ezStringView& sName) const
{
  for(const ezDocumentObjectBase* pChild : pObject->GetChildren())
  {
    const ezActionMapDescriptor* pDesc = GetDescriptor(pChild);
    if (sName.IsEqual_NoCase(pDesc->m_hAction.GetDescriptor()->m_sActionName.GetData()))
    {
      return pChild;
    }
  }
  return nullptr;
}

void ezActionMap::GetCreateableTypes(ezHybridArray<ezRTTI*, 32>& Types) const
{
  Types.PushBack(ezRTTI::FindTypeByName(ezGetStaticRTTI<ezActionMapDescriptor>()->GetTypeName()));
}


////////////////////////////////////////////////////////////////////////
// ezActionMap private functions
////////////////////////////////////////////////////////////////////////

ezDocumentObjectBase* ezActionMap::InternalCreateObject(const ezRTTI* pRtti)
{
  return new ezActionMap::ObjectType;
}

void ezActionMap::InternalDestroyObject(ezDocumentObjectBase* pObject)
{
  delete pObject;
}

bool ezActionMap::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const
{
  return true;
}

bool ezActionMap::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezActionMap::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  return true;
}
