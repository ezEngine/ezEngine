#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezDocumentObjectManager;
class ezDocumentObject;
class ezRTTI;

/// \brief Provides helper functions for serializing document object types and copying properties between objects.
///
/// Also check out ezToolsReflectionUtils for related functionality.
class EZ_TOOLSFOUNDATION_DLL ezToolsSerializationUtils
{
public:
  using FilterFunction = ezDelegate<bool(const ezAbstractProperty*)>;

  /// \brief Serializes the given set of types into the provided object graph.
  static void SerializeTypes(const ezSet<const ezRTTI*>& types, ezAbstractObjectGraph& ref_typesGraph);

  /// \brief Copies properties from a source document object to a target object, optionally filtering properties.
  static void CopyProperties(const ezDocumentObject* pSource, const ezDocumentObjectManager* pSourceManager, void* pTarget, const ezRTTI* pTargetType, FilterFunction propertFilter = nullptr);
};
