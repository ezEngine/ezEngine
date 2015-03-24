#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>


struct EZ_GUIFOUNDATION_DLL ezActionMapDescriptor
{
  ezActionDescriptorHandle m_hAction;  ///< Action to be mapped
  ezHashedString m_sPath; ///< Path where the action should be mapped excluding the action's name, e.g. "File/New" for a menu item "File -> New -> Project..." .
  float m_fOrder; ///< Ordering key to sort actions in the mapping path.
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezActionMapDescriptor);


class ezDocumentBase;

class ezActionObjectManager : public ezDocumentObjectManagerBase
{
public:
  ezActionObjectManager();
  virtual void GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const override;

private:
  virtual ezDocumentObjectBase* InternalCreateObject(ezReflectedTypeHandle hType) override;
  virtual void InternalDestroyObject(ezDocumentObjectBase* pObject) override;
  virtual bool InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const override;
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const override;
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const override;
};


class EZ_GUIFOUNDATION_DLL ezActionMap : public ezDocumentObjectTree
{
public:
  ezActionMap();
  ~ezActionMap();

  ezUuid MapAction(const ezActionMapDescriptor& desc);
  ezResult UnmapAction(const ezUuid& guid);

  bool FindObjectByPath(const ezStringView& sPath, ezUuid& out_guid) const;
  const ezActionMapDescriptor* GetDescriptor(const ezUuid& guid) const;
  const ezActionMapDescriptor* GetDescriptor(const ezDocumentObjectBase* pObject) const;
  const ezDocumentObjectBase* GetChildByName(const ezDocumentObjectBase* pObject, const ezStringView& sName) const;
  
  typedef ezDocumentObjectDirectMember<ezReflectedClass, ezActionMapDescriptor> ObjectType;
private:

private:
  ezActionObjectManager m_Manager;
  ezReflectedTypeHandle m_hDesc;
};
