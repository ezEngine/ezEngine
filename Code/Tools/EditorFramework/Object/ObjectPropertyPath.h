#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

class ezDocumentObject;
class ezObjectAccessorBase;

struct EZ_EDITORFRAMEWORK_DLL ezPropertyReference
{
  bool operator==(const ezPropertyReference& rhs) const
  {
    return m_Object == rhs.m_Object &&
      m_pProperty == rhs.m_pProperty &&
      m_Index == rhs.m_Index;
  }
  ezUuid m_Object;
  const ezAbstractProperty* m_pProperty;
  ezVariant m_Index;
};

struct EZ_EDITORFRAMEWORK_DLL ezObjectPropertyPathContext
{
  const ezDocumentObject* m_pContextObject; ///< Paths start at this object.
  ezObjectAccessorBase* m_pAccessor; ///< Accessor used to traverse hierarchy and query properties.
  ezString m_sRootProperty; ///< In case m_pContextObject points to the root object, this is the property to follow.
};

class EZ_EDITORFRAMEWORK_DLL ezObjectPropertyPath
{
public:
  static ezStatus CreatePath(const ezObjectPropertyPathContext& context, const ezPropertyReference& prop,
    ezStringBuilder& out_sObjectSearchSequence, ezStringBuilder& out_sComponentType, ezStringBuilder& out_sPropertyPath);
  static ezStatus CreatePropertyPath(const ezObjectPropertyPathContext& context, const ezPropertyReference& prop, ezStringBuilder& out_sPropertyPath);

  static ezStatus ResolvePath(const ezObjectPropertyPathContext& context, ezHybridArray<ezPropertyReference, 1>& out_keys,
    const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath);
  static ezStatus ResolvePropertyPath(const ezObjectPropertyPathContext& context, const char* szPropertyPath, ezPropertyReference& out_key);

  static const ezDocumentObject* FindParentNodeComponent(const ezDocumentObject* pObject);
private:
  static ezStatus PrependProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index, ezStringBuilder& out_sPropertyPath);
};
