#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezVisualShaderPinDescriptor
{
public:
  ezString m_sName;
  const ezRTTI* m_pDataType;
  ezReflectedPropertyDescriptor m_PropertyDesc;
  ezColorGammaUB m_Color;
};

class ezVisualShaderNodeDescriptor
{
public:

  ezString m_sName;
  ezColorGammaUB m_Color;

  ezHybridArray<ezVisualShaderPinDescriptor, 4> m_InputPins;
  ezHybridArray<ezVisualShaderPinDescriptor, 4> m_OutputPins;
  ezHybridArray<ezReflectedPropertyDescriptor, 4> m_Properties;
};


class ezVisualShaderTypeRegistry
{
  EZ_DECLARE_SINGLETON(ezVisualShaderTypeRegistry);

public:
  ezVisualShaderTypeRegistry();
  ~ezVisualShaderTypeRegistry();

  const ezVisualShaderNodeDescriptor* GetDescriptorForType(const ezRTTI* pRtti) const;

  const ezRTTI* GetNodeBaseType() const { return m_pBaseType; }

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, VisualShader);

  void LoadNodeData();
  const ezRTTI* GenerateTypeFromDesc(const ezVisualShaderNodeDescriptor& desc) const;

  ezMap<const ezRTTI*, ezVisualShaderNodeDescriptor> m_NodeDescriptors;

  const ezRTTI* m_pBaseType;
};
