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

ezActionMap::ezActionMap()
{
  //ezReflectedTypeDescriptor desc;
  //ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(ezGetStaticRTTI<ezActionMapDescriptor>(), desc);
  //m_pRtti = ezPhantomRttiManager::RegisterType(desc);
}

ezActionMap::~ezActionMap()
{
  //DestroyAllObjects();
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
  
  auto it = m_Descriptors.Find(ParentGUID);

  ezTreeNode<ezActionMapDescriptor>* pParent = nullptr;
  if (it.IsValid())
  {
    pParent = it.Value();
  }

  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
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

  if (GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName) != nullptr)
  {
    ezLog::Error("Can't map descriptor as its name is already present: %s", desc.m_hAction.GetDescriptor()->m_sActionName.GetData());
    return ezUuid();
  }

  ezInt32 iIndex = 0;
  for(iIndex = 0; iIndex < (ezInt32) pParent->GetChildren().GetCount(); ++iIndex)
  {
    const ezTreeNode<ezActionMapDescriptor>* pChild = pParent->GetChildren()[iIndex];
    const ezActionMapDescriptor* pDesc = GetDescriptor(pChild);

    if (desc.m_fOrder < pDesc->m_fOrder)
      break;
  }

  ezTreeNode<ezActionMapDescriptor>* pChild = pParent->InsertChild(desc, iIndex);

  m_Descriptors.Insert(pChild->GetGuid(), pChild);

  return pChild->GetGuid();
}

ezResult ezActionMap::UnmapAction(const ezUuid& guid)
{
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return EZ_FAILURE;

  ezTreeNode<ezActionMapDescriptor>* pNode = it.Value();
  m_Descriptors.Remove(it);
  pNode->GetParent()->RemoveChild(pNode->GetParentIndex());
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

  const ezTreeNode<ezActionMapDescriptor>* pParent = &m_Root;
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
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return nullptr;
  return GetDescriptor(it.Value());
}

const ezActionMapDescriptor* ezActionMap::GetDescriptor(const ezTreeNode<ezActionMapDescriptor>* pObject) const
{
  if (pObject == nullptr)
    return nullptr;

  return &pObject->m_Data;
}

const ezTreeNode<ezActionMapDescriptor>* ezActionMap::GetChildByName(const ezTreeNode<ezActionMapDescriptor>* pObject, const ezStringView& sName) const
{
  for(const ezTreeNode<ezActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const ezActionMapDescriptor& pDesc = pChild->m_Data;
    if (sName.IsEqual_NoCase(pDesc.m_hAction.GetDescriptor()->m_sActionName.GetData()))
    {
      return pChild;
    }
  }
  return nullptr;
}

