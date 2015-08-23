#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezAbstractObjectGraph;
class ezAbstractObjectNode;

struct EZ_FOUNDATION_DLL ezRttiConverterObject
{
  ezRttiConverterObject() : m_pType(nullptr), m_pObject(nullptr) {}
  ezRttiConverterObject(const ezRTTI* pType, void* pObject) : m_pType(pType), m_pObject(pObject) {}

  EZ_DECLARE_POD_TYPE();

  const ezRTTI* m_pType;
  void* m_pObject;
};


class EZ_FOUNDATION_DLL ezRttiConverterContext
{
public:
  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti);
  virtual void DeleteObject(const ezUuid& guid);

  virtual void RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject);
  virtual void UnregisterObject(const ezUuid& guid);

  virtual ezRttiConverterObject* GetObjectByGUID(const ezUuid& guid) const;
  virtual ezUuid GetObjectGUID(const ezRTTI* pRtti, void* pObject) const;

  virtual ezUuid EnqueObject(const ezRTTI* pRtti, void* pObject);
  virtual ezRttiConverterObject* DequeueObject();

private:
  ezHashTable<ezUuid, ezRttiConverterObject> m_GuidToObject;
  ezHashTable<void*, ezUuid> m_ObjectToGuid;
  ezSet<ezUuid> m_QueuedObjects;
};


class EZ_FOUNDATION_DLL ezRttiConverterWriter
{
public:

  ezRttiConverterWriter(ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs)
  {
    m_pGraph = pGraph;
    m_pContext = pContext;
    m_bSerializeReadOnly  = bSerializeReadOnly;
    m_bSerializeOwnerPtrs = bSerializeOwnerPtrs;
  }

  ezAbstractObjectNode* AddObjectToGraph(ezReflectedClass* pObject, const char* szNodeName = nullptr) { return AddObjectToGraph(pObject->GetDynamicRTTI(), pObject, szNodeName); }
  ezAbstractObjectNode* AddObjectToGraph(const ezRTTI* pRtti, void* pObject, const char* szNodeName = nullptr);

private:
  void AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const void* pObject);
  void AddProperties(ezAbstractObjectNode* pNode, const ezRTTI* pRtti, const void* pObject);

  ezAbstractObjectNode* AddSubObjectToGraph(const ezRTTI* pRtti, void* pObject, const ezUuid& guid, const char* szNodeName);

  ezRttiConverterContext* m_pContext;
  ezAbstractObjectGraph* m_pGraph;
  bool m_bSerializeReadOnly;
  bool m_bSerializeOwnerPtrs;
};

class EZ_FOUNDATION_DLL ezRttiConverterReader
{
public:

  ezRttiConverterReader(const ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext);

  void* CreateObjectFromNode(const ezAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, ezAbstractProperty* pProperty, const ezAbstractObjectNode::Property* pSource);

  ezRttiConverterContext* m_pContext;
  const ezAbstractObjectGraph* m_pGraph;
};

