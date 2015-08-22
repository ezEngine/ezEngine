#pragma once

/// \file

#include <Foundation/Basics.h>

class EZ_FOUNDATION_DLL ezPropertyAttribute : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAttribute);

public:

};

class EZ_FOUNDATION_DLL ezReadOnlyAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReadOnlyAttribute);

public:

};

class EZ_FOUNDATION_DLL ezHiddenAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHiddenAttribute);

public:

};

class EZ_FOUNDATION_DLL ezContainerAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezContainerAttribute);

public:
  ezContainerAttribute(){}
  ezContainerAttribute(bool bCanAdd, bool bCanDelete, bool bCanMove)
  {
    m_bCanAdd = bCanAdd;
    m_bCanDelete = bCanDelete;
    m_bCanMove = bCanMove;
  }

  bool CanAdd() const { return m_bCanAdd; }
  bool CanDelete() const { return m_bCanDelete; }
  bool CanMove() const { return m_bCanMove; }

private:
  bool m_bCanAdd;
  bool m_bCanDelete;
  bool m_bCanMove;
};

class EZ_FOUNDATION_DLL ezFileBrowserAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFileBrowserAttribute);

public:
  ezFileBrowserAttribute() {}
  ezFileBrowserAttribute(const char* szDialogTitle, const char* szTypeFilter)
  {
    m_sDialogTitle = szDialogTitle;
    m_sTypeFilter = szTypeFilter;
  }

  const char* GetDialogTitle() const { return m_sDialogTitle; }
  const char* GetTypeFilter() const { return m_sTypeFilter; }

private:
  EZ_ALLOW_PRIVATE_PROPERTIES(ezFileBrowserAttribute);

  ezString m_sDialogTitle;
  ezString m_sTypeFilter;
};


class EZ_FOUNDATION_DLL ezAssetBrowserAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetBrowserAttribute);

public:
  ezAssetBrowserAttribute() {}
  ezAssetBrowserAttribute(const char* szTypeFilter)
  {
    m_sTypeFilter = szTypeFilter;
  }

  const char* GetTypeFilter() const { return m_sTypeFilter; }

private:
  EZ_ALLOW_PRIVATE_PROPERTIES(ezAssetBrowserAttribute);

  ezString m_sTypeFilter;
};









