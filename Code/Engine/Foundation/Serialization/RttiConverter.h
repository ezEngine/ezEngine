#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezAbstractObjectGraph;
class ezAbstractObjectNode;

struct EZ_FOUNDATION_DLL ezRttiConverterObject
{
  ezRttiConverterObject()
    : m_pType(nullptr)
    , m_pObject(nullptr)
  {
  }
  ezRttiConverterObject(const ezRTTI* pType, void* pObject)
    : m_pType(pType)
    , m_pObject(pObject)
  {
  }

  EZ_DECLARE_POD_TYPE();

  const ezRTTI* m_pType;
  void* m_pObject;
};


class EZ_FOUNDATION_DLL ezRttiConverterContext
{
public:
  virtual void Clear();

  /// \brief Generates a guid for a new object. Default implementation generates stable guids derived from
  /// parentGuid + property name + index and ignores the address of pObject.
  virtual ezUuid GenerateObjectGuid(const ezUuid& parentGuid, const ezAbstractProperty* pProp, ezVariant index, void* pObject) const;

  virtual ezInternal::NewInstance<void> CreateObject(const ezUuid& guid, const ezRTTI* pRtti);
  virtual void DeleteObject(const ezUuid& guid);

  virtual void RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject);
  virtual void UnregisterObject(const ezUuid& guid);

  virtual ezRttiConverterObject GetObjectByGUID(const ezUuid& guid) const;
  virtual ezUuid GetObjectGUID(const ezRTTI* pRtti, const void* pObject) const;

  virtual const ezRTTI* FindTypeByName(ezStringView sName) const;

  template <typename T>
  void GetObjectsByType(ezDynamicArray<T*>& out_objects, ezDynamicArray<ezUuid>* out_pUuids = nullptr)
  {
    for (auto it : m_GuidToObject)
    {
      if (it.Value().m_pType->IsDerivedFrom(ezGetStaticRTTI<T>()))
      {
        out_objects.PushBack(static_cast<T*>(it.Value().m_pObject));
        if (out_pUuids)
        {
          out_pUuids->PushBack(it.Key());
        }
      }
    }
  }

  virtual ezUuid EnqueObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject);
  virtual ezRttiConverterObject DequeueObject();

  virtual void OnUnknownTypeError(ezStringView sTypeName);

protected:
  ezHashTable<ezUuid, ezRttiConverterObject> m_GuidToObject;
  mutable ezHashTable<const void*, ezUuid> m_ObjectToGuid;
  ezSet<ezUuid> m_QueuedObjects;
};


class EZ_FOUNDATION_DLL ezRttiConverterWriter
{
public:
  using FilterFunction = ezDelegate<bool(const void* pObject, const ezAbstractProperty* pProp)>;

  ezRttiConverterWriter(ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs);
  ezRttiConverterWriter(ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext, FilterFunction filter);

  ezAbstractObjectNode* AddObjectToGraph(ezReflectedClass* pObject, const char* szNodeName = nullptr)
  {
    return AddObjectToGraph(pObject->GetDynamicRTTI(), pObject, szNodeName);
  }
  ezAbstractObjectNode* AddObjectToGraph(const ezRTTI* pRtti, const void* pObject, const char* szNodeName = nullptr);

  void AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const void* pObject);
  void AddProperties(ezAbstractObjectNode* pNode, const ezRTTI* pRtti, const void* pObject);

  ezAbstractObjectNode* AddSubObjectToGraph(const ezRTTI* pRtti, const void* pObject, const ezUuid& guid, const char* szNodeName);

private:
  ezRttiConverterContext* m_pContext = nullptr;
  ezAbstractObjectGraph* m_pGraph = nullptr;
  FilterFunction m_Filter;
};

class EZ_FOUNDATION_DLL ezRttiConverterReader
{
public:
  ezRttiConverterReader(const ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext);

  ezInternal::NewInstance<void> CreateObjectFromNode(const ezAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, const ezAbstractProperty* pProperty, const ezAbstractObjectNode::Property* pSource);
  void CallOnObjectCreated(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject);

  ezRttiConverterContext* m_pContext = nullptr;
  const ezAbstractObjectGraph* m_pGraph = nullptr;
};
