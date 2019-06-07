#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/Action.h>
#include <QIcon>

///
class EZ_GUIFOUNDATION_DLL ezNamedAction : public ezAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNamedAction, ezAction);
public:
  ezNamedAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezAction(context), m_sName(szName), m_sIconPath(szIconPath) {}

  const char* GetName() const { return m_sName; }

  const char* GetAdditionalDisplayString() { return m_sAdditionalDisplayString; }
  void SetAdditionalDisplayString(const char* szString, bool bTriggerUpdate = true) { m_sAdditionalDisplayString = szString; if (bTriggerUpdate) TriggerUpdate(); }

  const char* GetIconPath() const { return m_sIconPath; }
  void SetIconPath(const char* szIconPath) { m_sIconPath = szIconPath; }

protected:
  ezString m_sName;
  ezString m_sAdditionalDisplayString; // to add some context to the current action
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
class EZ_GUIFOUNDATION_DLL ezDynamicMenuAction : public ezMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicMenuAction, ezMenuAction);
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
        Default = 0,
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

  ezDynamicMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezMenuAction(context, szName, szIconPath) {}
  virtual void GetEntries(ezHybridArray<Item, 16>& out_Entries) = 0;
};

///
class EZ_GUIFOUNDATION_DLL ezDynamicActionAndMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicActionAndMenuAction, ezDynamicMenuAction);
public:
  ezDynamicActionAndMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true) { m_bEnabled = bEnable; if (bTriggerUpdate) TriggerUpdate(); }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true) { m_bVisible = bVisible; if (bTriggerUpdate) TriggerUpdate(); }

protected:
  bool m_bEnabled;
  bool m_bVisible;
};

///
class EZ_GUIFOUNDATION_DLL ezEnumerationMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEnumerationMenuAction, ezDynamicMenuAction);
public:
  ezEnumerationMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  void InitEnumerationType(const ezRTTI* pEnumerationType);
  virtual void GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries) override;
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


class EZ_GUIFOUNDATION_DLL ezSliderAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSliderAction, ezNamedAction);
public:
  ezSliderAction(const ezActionContext& context, const char* szName);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true) { m_bEnabled = bEnable; if (bTriggerUpdate) TriggerUpdate(); }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true) { m_bVisible = bVisible; if (bTriggerUpdate) TriggerUpdate(); }

  void GetRange(ezInt32& out_iMin, ezInt32& out_iMax) const
  {
    out_iMin = m_iMinValue;
    out_iMax = m_iMaxValue;
  }

  void SetRange(ezInt32 iMin, ezInt32 iMax, bool bTriggerUpdate = true);

  ezInt32 GetValue() const { return m_iCurValue; }
  void SetValue(ezInt32 val, bool bTriggerUpdate = true);

protected:
  bool m_bEnabled;
  bool m_bVisible;
  ezInt32 m_iMinValue;
  ezInt32 m_iMaxValue;
  ezInt32 m_iCurValue;
};
