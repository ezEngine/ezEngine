#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezOpenDdlReaderElement;

class ezVisualShaderPinDescriptor
{
public:
  ezVisualShaderPinDescriptor()
  {
    m_bExposeAsProperty = false;
  }

  ezString m_sName;
  const ezRTTI* m_pDataType;
  ezReflectedPropertyDescriptor m_PropertyDesc;
  ezColorGammaUB m_Color;
  bool m_bExposeAsProperty;
  ezString m_sDefaultValue;
  ezString m_sShaderCodeInline;
  ezString m_sTooltip;
};

struct ezVisualShaderNodeType
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Generic,
    Main,
    Texture,

    Default = Generic
  };
};

class ezVisualShaderNodeDescriptor
{
public:

  ezEnum<ezVisualShaderNodeType> m_NodeType;
  ezString m_sName;
  ezString m_sCategory;
  ezColorGammaUB m_Color;
  ezString m_sShaderCodePixelDefines;
  ezString m_sShaderCodePixelIncludes;
  ezString m_sShaderCodePixelSamplers;
  ezString m_sShaderCodePixelConstants;
  ezString m_sShaderCodePixelBody;
  ezString m_sShaderCodePermutations;
  ezString m_sShaderCodeMaterialParams;
  ezString m_sShaderCodeRenderState;
  ezString m_sShaderCodeVertexShader;

  ezHybridArray<ezVisualShaderPinDescriptor, 4> m_InputPins;
  ezHybridArray<ezVisualShaderPinDescriptor, 4> m_OutputPins;
  ezHybridArray<ezReflectedPropertyDescriptor, 4> m_Properties;
};


class ezVisualShaderTypeRegistry
{
  EZ_DECLARE_SINGLETON(ezVisualShaderTypeRegistry);

public:
  ezVisualShaderTypeRegistry();

  const ezVisualShaderNodeDescriptor* GetDescriptorForType(const ezRTTI* pRtti) const;

  const ezRTTI* GetNodeBaseType() const { return m_pBaseType; }

  const ezRTTI* GetPinSamplerType() const { return m_pSamplerPinType; }

  void UpdateNodeData();

  void UpdateNodeData(const char* szCfgFileRelative);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, VisualShader);

  void LoadNodeData();
  const ezRTTI* GenerateTypeFromDesc(const ezVisualShaderNodeDescriptor& desc);
  void LoadConfigFile(const char* szFile);

  void ExtractNodePins(const ezVariantDictionary &varNodeDict, const char* szPinType, ezHybridArray<ezVisualShaderPinDescriptor, 4> &pinArray, bool bOutput);
  void ExtractNodePins(const ezOpenDdlReaderElement* pNode, const char* szPinType, ezHybridArray<ezVisualShaderPinDescriptor, 4> &pinArray, bool bOutput);
  void ExtractNodeProperties(const ezVariantDictionary &varNodeDict, ezVisualShaderNodeDescriptor &nd);
  void ExtractNodeProperties(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor &nd);
  void ExtractNodeConfig(const ezVariantDictionary &varNodeDict, ezVisualShaderNodeDescriptor &nd);
  void ExtractNodeConfig(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor &nd);


  ezMap<const ezRTTI*, ezVisualShaderNodeDescriptor> m_NodeDescriptors;

  const ezRTTI* m_pBaseType;
  const ezRTTI* m_pSamplerPinType;
};
