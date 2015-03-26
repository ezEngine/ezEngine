#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/Action.h>

///
class EZ_GUIFOUNDATION_DLL ezNamedAction : public ezAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNamedAction);
public:
  ezNamedAction(const ezActionContext& context, const char* szText) : ezAction(context), m_sText(szText) {}

  const char* GetText() const { return m_sText; }
  void SetText(const char* szName) { m_sText = szName; }

  const char* GetIconPath() const { return m_sIconPath; }
  void SetIconPath(const char* szIconPath) { m_sIconPath = szIconPath; }

protected:
  ezString m_sText;
  ezString m_sIconPath;
};

///
class EZ_GUIFOUNDATION_DLL ezCategoryAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCategoryAction);
public:
  ezCategoryAction(const ezActionContext& context, const char* szName) : ezNamedAction(context, szName) {}

  virtual void Execute(const ezVariant& value) override { };
};

///
class EZ_GUIFOUNDATION_DLL ezMenuAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMenuAction);
public:
  ezMenuAction(const ezActionContext& context, const char* szName) : ezNamedAction(context, szName) {}

  virtual void Execute(const ezVariant& value) override { };
};

///
class EZ_GUIFOUNDATION_DLL ezLRUMenuAction : public ezMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLRUMenuAction);
public:
  ezLRUMenuAction(const ezActionContext& context, const char* szName) : ezMenuAction(context, szName) {}
  virtual void GetEntries(ezHybridArray<std::pair<ezString, ezVariant>, 16>& out_Entries) = 0;
};

///
class EZ_GUIFOUNDATION_DLL ezButtonAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezButtonAction);
public:
  ezButtonAction(const ezActionContext& context, const char* szName, bool bCheckable);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable) { m_bEnabled = bEnable; }

  bool IsCheckable() const { return m_bCheckable; }
  void SetCheckable(bool bCheckable) { m_bCheckable = bCheckable; }

  bool IsChecked() const { return m_bChecked; }
  void SetChecked(bool bChecked) { m_bChecked = bChecked; }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible) { m_bVisible = bVisible; }

protected:
  bool m_bCheckable;
  bool m_bChecked;
  bool m_bEnabled;
  bool m_bVisible;
};