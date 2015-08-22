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









