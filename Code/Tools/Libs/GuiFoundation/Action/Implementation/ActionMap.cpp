#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezActionMapDescriptor, ezNoBase, 0, ezRTTINoAllocator);
//  EZ_BEGIN_PROPERTIES
//  EZ_END_PROPERTIES;
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezActionMap public functions
////////////////////////////////////////////////////////////////////////

ezActionMap::ezActionMap() = default;

ezActionMap::~ezActionMap() = default;

void ezActionMap::MapAction(ezActionDescriptorHandle hAction, ezStringView sPath, ezStringView sSubPath, float fOrder)
{
  ezStringBuilder sFullPath = sPath;

  if (!sPath.IsEmpty() && sPath.FindSubString("/") == nullptr)
  {
    if (SearchPathForAction(sPath, sFullPath).Failed())
    {
      sFullPath = sPath;
    }
  }

  sFullPath.AppendPath(sSubPath);

  MapAction(hAction, sFullPath, fOrder);
}

void ezActionMap::MapAction(ezActionDescriptorHandle hAction, ezStringView sPath, float fOrder)
{
  ezStringBuilder sCleanPath = sPath;
  sCleanPath.MakeCleanPath();
  sCleanPath.Trim("/");
  ezActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath = sCleanPath;
  d.m_fOrder = fOrder;

  if (!d.m_sPath.IsEmpty() && d.m_sPath.FindSubString("/") == nullptr)
  {
    ezStringBuilder sFullPath;
    if (SearchPathForAction(d.m_sPath, sFullPath).Succeeded())
    {
      d.m_sPath = sFullPath;
    }
  }

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
      ezLog::Error("Can't map descriptor '{0}' as its parent is an action itself and thus can't have any children.",
        desc.m_hAction.GetDescriptor()->m_sActionName);
      return ezUuid();
    }
  }

  if (GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName) != nullptr)
  {
    ezLog::Error("Can't map descriptor as its name is already present: {0}", desc.m_hAction.GetDescriptor()->m_sActionName);
    return ezUuid();
  }

  ezInt32 iIndex = 0;
  for (iIndex = 0; iIndex < (ezInt32)pParent->GetChildren().GetCount(); ++iIndex)
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
  if (ezTreeNode<ezActionMapDescriptor>* pParent = pNode->GetParent())
  {
    pParent->RemoveChild(pNode->GetParentIndex());
  }
  m_Descriptors.Remove(it);
  return EZ_SUCCESS;
}

ezResult ezActionMap::UnmapAction(ezActionDescriptorHandle hAction, ezStringView sPath)
{
  ezStringBuilder sCleanPath = sPath;
  sCleanPath.MakeCleanPath();
  sCleanPath.Trim("/");
  ezActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath = sCleanPath;
  d.m_fOrder = 0.0f; // unused.

  if (!d.m_sPath.IsEmpty() && d.m_sPath.FindSubString("/") == nullptr)
  {
    ezStringBuilder sFullPath;
    if (SearchPathForAction(d.m_sPath, sFullPath).Succeeded())
    {
      d.m_sPath = sFullPath;
    }
  }

  return UnmapAction(d);
}

ezResult ezActionMap::UnmapAction(const ezActionMapDescriptor& desc)
{
  ezTreeNode<ezActionMapDescriptor>* pParent = nullptr;
  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
  }
  else
  {
    ezUuid ParentGUID;
    if (!FindObjectByPath(desc.m_sPath, ParentGUID))
      return EZ_FAILURE;

    auto it = m_Descriptors.Find(ParentGUID);
    if (!it.IsValid())
      return EZ_FAILURE;

    pParent = it.Value();
  }

  if (auto* pChild = GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName))
  {
    return UnmapAction(pChild->GetGuid());
  }
  return EZ_FAILURE;
}

bool ezActionMap::FindObjectByPath(ezStringView sPath, ezUuid& out_guid) const
{
  out_guid = ezUuid();
  if (sPath.IsEmpty())
    return true;

  ezStringBuilder sPathBuilder(sPath);
  ezHybridArray<ezStringView, 8> parts;
  sPathBuilder.Split(false, parts, "/");

  const ezTreeNode<ezActionMapDescriptor>* pParent = &m_Root;
  for (const ezStringView& name : parts)
  {
    pParent = GetChildByName(pParent, name);
    if (pParent == nullptr)
      return false;
  }

  out_guid = pParent->GetGuid();
  return true;
}

ezResult ezActionMap::SearchPathForAction(ezStringView sUniqueName, ezStringBuilder& out_sPath) const
{
  out_sPath.Clear();

  if (FindObjectPathByName(&m_Root, sUniqueName, out_sPath))
  {
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

bool ezActionMap::FindObjectPathByName(const ezTreeNode<ezActionMapDescriptor>* pObject, ezStringView sName, ezStringBuilder& out_sPath) const
{
  ezStringView sObjectName;

  if (!pObject->m_Data.m_hAction.IsInvalidated())
  {
    sObjectName = pObject->m_Data.m_hAction.GetDescriptor()->m_sActionName;
  }

  out_sPath.AppendPath(sObjectName);

  if (sObjectName == sName)
    return true;

  for (const ezTreeNode<ezActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const ezActionMapDescriptor& pDesc = pChild->m_Data;

    if (FindObjectPathByName(pChild, sName, out_sPath))
      return true;
  }

  out_sPath.PathParentDirectory();
  return false;
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

const ezTreeNode<ezActionMapDescriptor>* ezActionMap::GetChildByName(const ezTreeNode<ezActionMapDescriptor>* pObject, ezStringView sName) const
{
  for (const ezTreeNode<ezActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const ezActionMapDescriptor& pDesc = pChild->m_Data;
    if (sName.IsEqual_NoCase(pDesc.m_hAction.GetDescriptor()->m_sActionName.GetData()))
    {
      return pChild;
    }
  }
  return nullptr;
}
