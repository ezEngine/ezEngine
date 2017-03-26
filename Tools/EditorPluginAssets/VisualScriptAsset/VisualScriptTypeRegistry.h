#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

struct ezVisualScriptPinDescriptor
{
  enum PinType { Execution, Data };

  ezString m_sName;
  const ezRTTI* m_pDataType = nullptr;
  //ezReflectedPropertyDescriptor m_PropertyDesc;
  ezColorGammaUB m_Color;
  //bool m_bExposeAsProperty = false;
  //ezString m_sDefaultValue;
  ezString m_sTooltip;

  PinType m_PinType;
  ezUInt8 m_uiPinIndex;
};

struct ezVisualScriptNodeDescriptor
{
  ezString m_sTypeName;
  ezString m_sCategory;
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

  void UpdateNodeData();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, VisualScript);

  void LoadNodeData();
  const ezRTTI* GenerateTypeFromDesc(const ezVisualScriptNodeDescriptor& desc);

    ezMap<const ezRTTI*, ezVisualScriptNodeDescriptor> m_NodeDescriptors;

  const ezRTTI* m_pBaseType;
};
