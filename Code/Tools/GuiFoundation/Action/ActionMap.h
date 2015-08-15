#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezDocumentBase;

struct EZ_GUIFOUNDATION_DLL ezActionMapDescriptor
{
  ezActionDescriptorHandle m_hAction;  ///< Action to be mapped
  ezString m_sPath; ///< Path where the action should be mapped excluding the action's name, e.g. "File/New" for a menu item "File -> New -> Project..." .
  float m_fOrder; ///< Ordering key to sort actions in the mapping path.
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezActionMapDescriptor);

template<typename T>
class ezTreeNode
{
public:
  ezTreeNode() : m_pParent(nullptr) {}
  ezTreeNode(const T& data) : m_Data(data), m_pParent(nullptr) {}
  ~ezTreeNode()
  {
    while (!m_Children.IsEmpty())
    {
      RemoveChild(0);
    }
  }

  const ezUuid& GetGuid() const { return m_Guid; }
  const ezTreeNode<T>* GetParent() const { return m_pParent; }
  ezTreeNode<T>* GetParent() { return m_pParent; }
  const ezHybridArray<ezTreeNode<T>*, 8>& GetChildren() const { return m_Children; }
  ezHybridArray<ezTreeNode<T>*, 8>& GetChildren() { return m_Children; }
  
  ezTreeNode<T>* InsertChild(const T& data, ezUInt32 iIndex)
  {
    ezTreeNode<T>* pNode = EZ_DEFAULT_NEW(ezTreeNode<T>, data);
    pNode->m_Guid.CreateNewUuid();
    m_Children.Insert(pNode, iIndex);
    return pNode;
  }

  bool RemoveChild(ezUInt32 iIndex)
  {
    if (iIndex > m_Children.GetCount())
      return false;

    ezTreeNode<T>* pChild = m_Children[iIndex];
    m_Children.RemoveAt(iIndex);
    EZ_DEFAULT_DELETE(pChild);
    return true;
  }

  ezUInt32 GetParentIndex() const
  {
    EZ_ASSERT_DEV(m_pParent != nullptr, "Can't compute parent index if no parent is present!");
    for (ezUInt32 i = 0; i < m_pParent->GetChildren().GetCount(); i++)
    {
      if (m_pParent->GetChildren()[i] == this)
        return i;
    }
    EZ_REPORT_FAILURE("Couldn't find oneself in own parent!");
    return -1;
  }

  T m_Data;
  ezUuid m_Guid;

private:
  ezTreeNode<T>* m_pParent;
  ezHybridArray<ezTreeNode<T>*, 8> m_Children;
};


class EZ_GUIFOUNDATION_DLL ezActionMap
{
public:
  typedef ezTreeNode<ezActionMapDescriptor> TreeNode;
  ezActionMap();
  ~ezActionMap();

  void MapAction(ezActionDescriptorHandle hAction, const char* szPath, float m_fOrder);
  ezUuid MapAction(const ezActionMapDescriptor& desc);
  ezResult UnmapAction(const ezUuid& guid);

  const TreeNode* GetRootObject() const { return &m_Root; }
  const ezActionMapDescriptor* GetDescriptor(const ezUuid& guid) const;
  const ezActionMapDescriptor* GetDescriptor(const ezTreeNode<ezActionMapDescriptor>* pObject) const;
  
private:
  bool FindObjectByPath(const ezStringView& sPath, ezUuid& out_guid) const;
  const ezTreeNode<ezActionMapDescriptor>* GetChildByName(const ezTreeNode<ezActionMapDescriptor>* pObject, const ezStringView& sName) const;

private:
  TreeNode m_Root;
  ezMap<ezUuid, ezTreeNode<ezActionMapDescriptor>*> m_Descriptors;
  const ezRTTI* m_pRtti;
};

