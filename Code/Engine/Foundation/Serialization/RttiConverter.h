#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezAbstractObjectGraph;
class ezAbstractObjectNode;

/// \brief Simple wrapper that pairs a runtime type with an object instance pointer.
///
/// This structure is used throughout the RTTI converter system to maintain type safety
/// when working with void pointers. It ensures that object pointers are always associated
/// with their correct runtime type information.
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

  const ezRTTI* m_pType; ///< Runtime type information for the object
  void* m_pObject;       ///< Pointer to the actual object instance
};


/// \brief Context object that manages object lifetime and relationships during RTTI-based conversion.
///
/// This class provides the infrastructure for converting between native objects and abstract
/// object graphs. It handles object creation, deletion, GUID management, and type resolution
/// during both serialization and deserialization processes.
///
/// Key responsibilities:
/// - Object lifecycle management (creation, registration, deletion)
/// - GUID generation and object-to-GUID mapping
/// - Type resolution and unknown type handling
/// - Object queuing for deferred processing
/// - Cross-reference resolution during deserialization
///
/// The context can be customized by overriding virtual methods to implement:
/// - Custom GUID generation strategies
/// - Alternative object creation patterns
/// - Specialized type resolution logic
/// - Custom error handling for unknown types
class EZ_FOUNDATION_DLL ezRttiConverterContext
{
public:
  /// \brief Clears all cached objects and resets the context state.
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


/// \brief Converts native objects to abstract object graph representation using reflection.
///
/// This class traverses object hierarchies using RTTI and converts them into abstract
/// object graphs that can be serialized to various formats. It handles object references,
/// inheritance hierarchies, and complex property types automatically.
class EZ_FOUNDATION_DLL ezRttiConverterWriter
{
public:
  /// \brief Filter function type for controlling which properties are serialized.
  ///
  /// Return true to include the property, false to skip it. Allows fine-grained control
  /// over what gets serialized based on object state, property attributes, or other criteria.
  using FilterFunction = ezDelegate<bool(const void* pObject, const ezAbstractProperty* pProp)>;

  /// \brief Constructs a writer with boolean flags for common filtering options.
  ///
  /// \param bSerializeReadOnly If true, includes read-only properties in the output
  /// \param bSerializeOwnerPtrs If true, serializes objects pointed to by owner pointers
  ezRttiConverterWriter(ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs);

  /// \brief Constructs a writer with a custom filter function for maximum control.
  ///
  /// The filter function is called for each property and can implement complex logic
  /// to determine what should be serialized.
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

/// \brief Converts abstract object graphs back to native objects using reflection.
///
/// This class performs the reverse operation of ezRttiConverterWriter, reconstructing
/// native object hierarchies from abstract object graphs. It handles object creation,
/// property restoration, and reference resolution automatically.
class EZ_FOUNDATION_DLL ezRttiConverterReader
{
public:
  /// \brief Constructs a reader for the given object graph and context.
  ezRttiConverterReader(const ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext);

  ezInternal::NewInstance<void> CreateObjectFromNode(const ezAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, const ezAbstractProperty* pProperty, const ezAbstractObjectNode::Property* pSource);
  void CallOnObjectCreated(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject);

  ezRttiConverterContext* m_pContext = nullptr;
  const ezAbstractObjectGraph* m_pGraph = nullptr;
};
