#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezDocument;

struct EZ_GUIFOUNDATION_DLL ezActionMapDescriptor
{
  ezActionDescriptorHandle m_hAction; ///< Action to be mapped
  ezString m_sPath;                   ///< Path where the action should be mapped excluding the action's name, e.g. "File/New" for a menu item "File -> New -> Project..." .
  float m_fOrder;                     ///< Ordering key to sort actions in the mapping path.
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezActionMapDescriptor);

template <typename T>
class ezTreeNode
{
public:
  ezTreeNode()
    : m_pParent(nullptr)
  {
  }
  ezTreeNode(const T& data)
    : m_Data(data)
    , m_pParent(nullptr)
  {
  }
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

  ezTreeNode<T>* InsertChild(const T& data, ezUInt32 uiIndex)
  {
    ezTreeNode<T>* pNode = EZ_DEFAULT_NEW(ezTreeNode<T>, data);
    pNode->m_Guid = ezUuid::MakeUuid();
    m_Children.InsertAt(uiIndex, pNode);
    pNode->m_pParent = this;
    return pNode;
  }

  bool RemoveChild(ezUInt32 uiIndex)
  {
    if (uiIndex > m_Children.GetCount())
      return false;

    ezTreeNode<T>* pChild = m_Children[uiIndex];
    m_Children.RemoveAtAndCopy(uiIndex);
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

/// \brief Defines the structure of how actions are organized in a particular context.
///
/// Actions are usually commands that are exposed through UI.
/// For instance a button in a toolbar or a menu entry.
///
/// Actions are unique. Each action only exists once in ezActionManager.
///
/// An action map defines where in a menu an action shows up.
/// Actions are usually grouped by categories. So for example all actions related to opening, closing
/// or saving a document may be in one group. Their position within that group is defined through
/// an 'order' value. This allows plugins to insert actions easily.
///
/// A window might use multiple action maps to build different structures.
/// For example, usually there is one action map for a window menu, and another map for a toolbar.
/// These will contain different actions, and they are organized differently.
///
/// Action maps are created through ezActionMapManager and are simply identified by name.
class EZ_GUIFOUNDATION_DLL ezActionMap
{
public:
  using TreeNode = ezTreeNode<ezActionMapDescriptor>;
  ezActionMap(ezStringView sParentMapping);
  ~ezActionMap();

  /// \brief Adds the given action to into the category or menu identified by sPath.
  ///
  /// All actions added to the same path will be sorted by 'fOrder' and the ones with the smaller values show up at the top.
  ///
  /// sPath must either be a fully qualified path OR the name of a uniquely named category or menu.
  /// If sPath is empty, the action (which may be a category itself) will be mapped into the root.
  /// This is common for top-level menus and for toolbars.
  ///
  /// If sPath is a fully qualified path, the segments are separated by slashes (/)
  /// and each segment must name either a category (see EZ_REGISTER_CATEGORY) or a menu (see EZ_REGISTER_MENU).
  ///
  /// sPath may also name a category or menu WITHOUT it being a full path. In this case the name must be unique.
  /// If sPath isn't empty and doesn't contain a slash, the system searches all available actions that are already in the action map.
  /// This allows you to insert an action into a category, without knowing the full path to that category.
  /// By convention, categories that are meant to be used that way are named "G.Something". The idea is, that where that category
  /// really shows up (and whether it is its own menu or just an area somewhere) may change in the future, or may be different
  /// in different contexts.
  ///
  /// To make it easier to use 'global' category names combined with an additional relative path, there is an overload of this function
  /// that takes an additional sSubPath argument.
  void MapAction(ezActionDescriptorHandle hAction, ezStringView sPath, float fOrder);

  /// \brief An overload of MapAction that takes a dedicated sPath and sSubPath argument for convenience.
  ///
  /// If sPath is a 'global' name of a category, it is searched for (see SearchPathForAction()).
  /// Afterwards sSubPath is appended and the result is forwarded to MapAction() as a single path string.
  void MapAction(ezActionDescriptorHandle hAction, ezStringView sPath, ezStringView sSubPath, float fOrder);

  /// \brief Hides an action from the action map. The same rules for 'global' names apply as for MapAction().
  /// If the target action is in this mapping, prefer not calling MapAction in the first place. Use this for actions to be removed that might be in a parent mapping and thus can't be modified directly.
  void HideAction(ezActionDescriptorHandle hAction, ezStringView sPath);

  /// \brief Builds an action tree out of all mapped actions of this and any parent mappings.
  const TreeNode* BuildActionTree();
  const ezActionMapDescriptor* GetDescriptor(const ezTreeNode<ezActionMapDescriptor>* pObject) const;

private:
  struct TempActionMapDescriptor
  {
    ezActionDescriptorHandle m_hAction;
    ezString m_sPath;
    ezString m_sSubPath;
    float m_fOrder;
  };

  /// \brief Searches for an action with the given name and returns the full path to it.
  ///
  /// This is mainly meant to be used with (unique) names to categories (or menus).
  ezResult SearchPathForAction(ezStringView sUniqueName, ezStringBuilder& out_sPath) const;

  void MapActionInternal(ezActionDescriptorHandle hAction, ezStringView sPath, float fOrder);
  void MapActionInternal(ezActionDescriptorHandle hAction, ezStringView sPath, ezStringView sSubPath, float fOrder);
  ezResult UnmapActionInternal(ezActionDescriptorHandle hAction, ezStringView sPath);

  ezUuid MapActionInternal(const ezActionMapDescriptor& desc);
  ezResult UnmapActionInternal(const ezActionMapDescriptor& desc);
  ezResult UnmapActionInternal(const ezUuid& guid);

  const ezActionMapDescriptor* GetDescriptor(const ezUuid& guid) const;

  bool FindObjectByPath(ezStringView sPath, ezUuid& out_guid) const;
  bool FindObjectPathByName(const ezTreeNode<ezActionMapDescriptor>* pObject, ezStringView sName, ezStringBuilder& out_sPath) const;
  const ezTreeNode<ezActionMapDescriptor>* GetChildByName(const ezTreeNode<ezActionMapDescriptor>* pObject, ezStringView sName) const;

private:
  ezString m_sParentMapping;
  ezDynamicArray<TempActionMapDescriptor> m_TempActions;
  ezDynamicArray<TempActionMapDescriptor> m_TempHiddenActions;
  ezUInt32 m_uiEditCounter = 0;

  mutable ezUInt32 m_uiTransitiveEditCounterOfRoot = 0;
  mutable TreeNode m_Root;
  mutable ezMap<ezUuid, ezTreeNode<ezActionMapDescriptor>*> m_Descriptors;
};
