#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct ezVisualScriptPinDescriptor
{
  enum PinType { Execution, Data };

  ezString m_sName;
  ezString m_sTooltip;
  ezColorGammaUB m_Color;
  ezVisualScriptDataPinType::Enum m_DataType;
  PinType m_PinType;
  ezUInt8 m_uiPinIndex;
};

struct ezVisualScriptNodeDescriptor
{
  ezString m_sTypeName;
  ezString m_sCategory;
  ezString m_sTitle;
  ezColorGammaUB m_Color;

  ezHybridArray<ezVisualScriptPinDescriptor, 4> m_InputPins;
  ezHybridArray<ezVisualScriptPinDescriptor, 4> m_OutputPins;
  ezHybridArray<ezReflectedPropertyDescriptor, 4> m_Properties;
};

class ezVisualScriptTypeRegistry
{
  EZ_DECLARE_SINGLETON(ezVisualScriptTypeRegistry);

public:
  ezVisualScriptTypeRegistry();

  const ezVisualScriptNodeDescriptor* GetDescriptorForType(const ezRTTI* pRtti) const;

  const ezRTTI* GetNodeBaseType() const { return m_pBaseType; }

  void UpdateNodeTypes();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, VisualScript);

  void PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e);
  void UpdateNodeType(const ezRTTI* pRtti);
  const ezRTTI* GenerateTypeFromDesc(const ezVisualScriptNodeDescriptor& desc);
  void CreateMessageNodeType(const ezRTTI* pRtti);

    ezMap<const ezRTTI*, ezVisualScriptNodeDescriptor> m_NodeDescriptors;

  const ezRTTI* m_pBaseType;
};
