#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/Action.h>

///
class EZ_GUIFOUNDATION_DLL ezNamedAction : public ezAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNamedAction);
public:
  ezNamedAction(const char* szText) : ezAction(), m_sText(szText) {}

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
  ezCategoryAction(const char* szName) : ezNamedAction(szName) {}

  virtual ezResult Init(const ezActionContext& context) override { return EZ_SUCCESS; };
  virtual ezResult Execute(const ezVariant& value) override { return EZ_SUCCESS; };
};

///
class EZ_GUIFOUNDATION_DLL ezMenuAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMenuAction);
public:
  ezMenuAction(const char* szName) : ezNamedAction(szName) {}

  virtual ezResult Init(const ezActionContext& context) override { return EZ_SUCCESS; };
  virtual ezResult Execute(const ezVariant& value) override { return EZ_SUCCESS; };
};

///
class EZ_GUIFOUNDATION_DLL ezButtonAction : public ezNamedAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezButtonAction);
public:
  ezButtonAction(const char* szName, bool bCheckable) : ezNamedAction(szName) {}

  bool IsCheckable() const { return m_bCheckable; }
  void SetCheckable(bool bCheckable) { m_bCheckable = bCheckable; }

  bool IsChecked() const { return m_bChecked; }
  void SetChecked(bool bChecked) { m_bChecked = bChecked; }

protected:
  bool m_bCheckable;
  bool m_bChecked;
};