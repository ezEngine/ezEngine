#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/Reflection.h>

/// An input pin of a node in the AI controller graph, e.g. the sensors that a perception generator reads.
///
/// Used as a reflected member of a sensor, perception generator, behavior or consideration. The member needs an ezAiInputPinAttribute,
/// which defines what can be connected, and an ezHiddenAttribute, since only the asset transform writes it.
/// The editor creates one pin per such member (named like the property), any number of nodes can be connected.
///
/// Holds the indices of the connected nodes in the ezAiControllerDesc, in connection order.
/// ezAiControllerDesc::Finalize() removes indices that are out of range or reference a node of the wrong type.
class EZ_AIPLUGIN_DLL ezAiInputPin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiInputPin, ezReflectedClass);

public:
  ezArrayPtr<const ezUInt32> GetConnections() const { return m_Connections; }

  ezSmallArray<ezUInt32, 2> m_Connections; // [ property ]
};

/// Defines which nodes can be connected to an ezAiInputPin member.
///
/// The type name is one of:
///  - a sensor type: accepts sensors of that type, the indices refer to ezAiControllerDesc::GetSensors().
///  - a perception type: accepts perception generators that produce that type (see ezAiPerceptionTypeAttribute),
///    the indices refer to ezAiControllerDesc::GetPerceptionGenerators().
///  - a consideration type: accepts considerations of that type, the indices refer to ezAiControllerDesc::GetConsiderations().
class EZ_AIPLUGIN_DLL ezAiInputPinAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiInputPinAttribute, ezPropertyAttribute);

public:
  ezAiInputPinAttribute() = default;
  ezAiInputPinAttribute(const char* szNodeType)
    : m_sNodeType(szNodeType)
  {
  }

  /// Returns nullptr if the type doesn't exist.
  const ezRTTI* GetNodeType() const;

  ezStringView GetNodeTypeName() const { return m_sNodeType; }

  /// Whether a node of the given type can be connected. False if the accepted type doesn't exist.
  bool Accepts(const ezRTTI* pNodeType) const;

private:
  ezUntrackedString m_sNodeType;
};
