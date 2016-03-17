#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <QIcon>

///
class EZ_GUIFOUNDATION_DLL ezNamedAction : public ezAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNamedAction, ezAction);
public:
  ezNamedAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezAction(context), m_sName(szName), m_sIconPath(szIconPath) {}

  const char* GetName() const { return m_sName; }

  const char* GetIconPath() const { return m_sIconPath; }
  void SetIconPath(const char* szIconPath) { m_sIconPath = szIconPath; }

protected:
  ezString m_sName;
  ezString m_sIconPath;
};

///
class EZ_GUIFOUNDATION_DLL ezCategoryAction : public ezAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCategoryAction, ezAction);
public:
  ezCategoryAction(const ezActionContext& context) : ezAction(context) {}

  virtual void Execute(const ezVariant& value) override { };
};

///
class EZ_GUIFOUNDATION_DLL ezMenuAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMenuAction, ezNamedAction);
public:
  ezMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezNamedAction(context, szName, szIconPath) {}

  virtual void Execute(const ezVariant& value) override { };
};

///
class EZ_GUIFOUNDATION_DLL ezLRUMenuAction : public ezMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLRUMenuAction, ezMenuAction);
public:
  struct Item
  {
    enum class CheckMark
    {
      NotCheckable,
      Unchecked,
      Checked
    };

    struct ItemFlags
    {
      typedef ezUInt8 StorageType;

      enum Enum
      {
        Separator = EZ_BIT(0),
      };
      struct Bits
      {
        StorageType Separator : 1;
      };
    };

    Item()
    {
      m_CheckState = CheckMark::NotCheckable;
    }

    ezString m_sDisplay;
    QIcon m_Icon;
    CheckMark m_CheckState;
    ezBitflags<ItemFlags> m_ItemFlags;
    ezVariant m_UserValue;
  };

  ezLRUMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezMenuAction(context, szName, szIconPath) {}
  virtual void GetEntries(ezHybridArray<Item, 16>& out_Entries) = 0;
};

///
class EZ_GUIFOUNDATION_DLL ezEnumerationMenuAction : public ezLRUMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEnumerationMenuAction, ezLRUMenuAction);
public:
  ezEnumerationMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  void InitEnumerationType(const ezRTTI* pEnumerationType);
  virtual void GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries) override;
  virtual ezInt64 GetValue() const = 0;

protected:
  const ezRTTI* m_pEnumerationType;
};

///
class EZ_GUIFOUNDATION_DLL ezButtonAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezButtonAction, ezNamedAction);
public:
  ezButtonAction(const ezActionContext& context, const char* szName, bool bCheckable, const char* szIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true) { m_bEnabled = bEnable; if (bTriggerUpdate) TriggerUpdate(); }

  bool IsCheckable() const { return m_bCheckable; }
  void SetCheckable(bool bCheckable, bool bTriggerUpdate = true) { m_bCheckable = bCheckable; if (bTriggerUpdate) TriggerUpdate(); }

  bool IsChecked() const { return m_bChecked; }
  void SetChecked(bool bChecked, bool bTriggerUpdate = true) { m_bChecked = bChecked; if (bTriggerUpdate) TriggerUpdate(); }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true) { m_bVisible = bVisible; if (bTriggerUpdate) TriggerUpdate(); }

protected:
  bool m_bCheckable;
  bool m_bChecked;
  bool m_bEnabled;
  bool m_bVisible;
};