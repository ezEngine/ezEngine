#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezDocumentObjectManager;
class ezDocumentObject;
class ezRTTI;

/// \brief Helper functions for serializing data
///
/// Also check out ezToolsReflectionUtils for related functionality.
class EZ_TOOLSFOUNDATION_DLL ezToolsSerializationUtils
{
public:
  using FilterFunction = ezDelegate<bool(const ezAbstractProperty*)>;

  static void SerializeTypes(const ezSet<const ezRTTI*>& types, ezAbstractObjectGraph& ref_typesGraph);

  static void CopyProperties(const ezDocumentObject* pSource, const ezDocumentObjectManager* pSourceManager, void* pTarget, const ezRTTI* pTargetType, FilterFunction propertFilter = nullptr);
};
