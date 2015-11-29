#pragma once

/// \file

#include <Foundation/Basics.h>

/// \brief Base class of all attributes can be used to decorate a RTTI property.
class EZ_FOUNDATION_DLL ezPropertyAttribute : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAttribute, ezReflectedClass);
};

class EZ_FOUNDATION_DLL ezReadOnlyAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReadOnlyAttribute, ezPropertyAttribute);
};

class EZ_FOUNDATION_DLL ezHiddenAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHiddenAttribute, ezPropertyAttribute);
};

/// \brief Sets the default value of the property.
class EZ_FOUNDATION_DLL ezDefaultValueAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDefaultValueAttribute, ezPropertyAttribute);

public:
  ezDefaultValueAttribute() {}
  ezDefaultValueAttribute(const ezVariant& value)
  {
    m_Value = value;
  }

  const ezVariant& GetValue() const { return m_Value; }

private:
  ezVariant m_Value;
};


class EZ_FOUNDATION_DLL ezClampValueAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClampValueAttribute, ezPropertyAttribute);

public:
  ezClampValueAttribute() {}
  ezClampValueAttribute(const ezVariant& min, const ezVariant& max)
  {
    m_MinValue = min;
    m_MaxValue = max;
  }

  const ezVariant& GetMinValue() const { return m_MinValue; }
  const ezVariant& GetMaxValue() const { return m_MaxValue; }

private:
  ezVariant m_MinValue;
  ezVariant m_MaxValue;
};


/// \brief Derive from this class if you want to define an attribute that replaces the property type widget.
///
/// Using this attribute affects both member properties as well as elements in a container but not the container widget.
/// When creating a property widget, the property grid will look for an attribute of this type and use
/// its type to look for a factory creator in ezRttiMappedObjectFactory<ezQtPropertyWidget>.
/// E.g. ezRttiMappedObjectFactory<ezQtPropertyWidget>::RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), FileBrowserCreator);
/// will replace the property widget for all properties that use ezFileBrowserAttribute.
class EZ_FOUNDATION_DLL ezTypeWidgetAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeWidgetAttribute, ezPropertyAttribute);
};

/// \brief Derive from this class if you want to define an attribute that replaces the property widget of containers.
///
/// Using this attribute affects the container widget but not container elements.
/// Only derive from this class if you want to replace the container widget itself, in every other case
/// prefer to use ezTypeWidgetAttribute.
class EZ_FOUNDATION_DLL ezContainerWidgetAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezContainerWidgetAttribute, ezPropertyAttribute);
};

/// \brief Add this attribute to a tag set member property to make it use the tag set editor
/// and define the categories it will use as a ; separated list of category names.
///
/// Usage: EZ_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new ezTagSetWidgetAttribute("Category1;Category2")),
class EZ_FOUNDATION_DLL ezTagSetWidgetAttribute : public ezContainerWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTagSetWidgetAttribute, ezContainerWidgetAttribute);

public:
  ezTagSetWidgetAttribute() {}
  ezTagSetWidgetAttribute(const char* szTagFilter)
  {
    m_sTagFilter = szTagFilter;
  }

  const char* GetTagFilter() const { return m_sTagFilter; }

private:
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTagSetWidgetAttribute);

  ezString m_sTagFilter;
};

/// \brief Sets the allowed actions on a container.
class EZ_FOUNDATION_DLL ezContainerAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezContainerAttribute, ezPropertyAttribute);

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

class EZ_FOUNDATION_DLL ezFileBrowserAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFileBrowserAttribute, ezTypeWidgetAttribute);

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


class EZ_FOUNDATION_DLL ezAssetBrowserAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetBrowserAttribute, ezTypeWidgetAttribute);

public:
  ezAssetBrowserAttribute() {}
  ezAssetBrowserAttribute(const char* szTypeFilter)
  {
    ezStringBuilder sTemp(";", szTypeFilter, ";");
    m_sTypeFilter = sTemp;
  }

  const char* GetTypeFilter() const { return m_sTypeFilter; }

private:
  EZ_ALLOW_PRIVATE_PROPERTIES(ezAssetBrowserAttribute);

  ezString m_sTypeFilter;
};









