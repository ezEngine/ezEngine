#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezOpenDdlReaderElement;

struct ezVisualShaderPinDescriptor
{
  ezString m_sName;
  const ezRTTI* m_pDataType = nullptr;
  ezReflectedPropertyDescriptor m_PropertyDesc;
  ezColorGammaUB m_Color = ezColorScheme::DarkUI(ezColorScheme::Gray);
  bool m_bExposeAsProperty = false;
  ezString m_sDefaultValue;
  ezDynamicArray<ezString> m_sDefinesWhenUsingDefaultValue;
  ezString m_sShaderCodeInline;
  ezString m_sTooltip;
};

struct ezVisualShaderNodeType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Generic,
    Main,
    Texture,

    Default = Generic
  };
};

struct ezVisualShaderNodeDescriptor
{
  ezEnum<ezVisualShaderNodeType> m_NodeType;
  ezString m_sCfgFile; ///< from which config file this node type was loaded
  ezString m_sName;
  ezHashedString m_sCategory;
  ezString m_sCheckPermutations;
  ezColorGammaUB m_Color = ezColorScheme::DarkUI(ezColorScheme::Gray);
  ezString m_sShaderCodePixelDefines;
  ezString m_sShaderCodePixelIncludes;
  ezString m_sShaderCodePixelSamplers;
  ezString m_sShaderCodePixelConstants;
  ezString m_sShaderCodePixelBody;
  ezString m_sShaderCodePermutations;
  ezString m_sShaderCodeMaterialParams;
  ezString m_sShaderCodeMaterialConstants;
  ezString m_sShaderCodeMaterialCB;
  ezString m_sShaderCodeRenderState;
  ezString m_sShaderCodeVertexShader;
  ezString m_sShaderCodeGeometryShader;

  ezHybridArray<ezVisualShaderPinDescriptor, 4> m_InputPins;
  ezHybridArray<ezVisualShaderPinDescriptor, 4> m_OutputPins;
  ezHybridArray<ezReflectedPropertyDescriptor, 4> m_Properties;
  ezHybridArray<ezInt8, 4> m_UniquePropertyValueGroups; // no property in the same group may share the same value, -1 for disabled
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

  void UpdateNodeData(ezStringView sCfgFileRelative);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorPluginAssets, VisualShader);

  void LoadNodeData();
  const ezRTTI* GenerateTypeFromDesc(const ezVisualShaderNodeDescriptor& desc);
  void LoadConfigFile(const char* szFile);

  void ExtractNodePins(
    const ezOpenDdlReaderElement* pNode, const char* szPinType, ezHybridArray<ezVisualShaderPinDescriptor, 4>& pinArray, bool bOutput);
  void ExtractNodeProperties(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor& nd);
  void ExtractNodeConfig(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor& nd);


  ezMap<const ezRTTI*, ezVisualShaderNodeDescriptor> m_NodeDescriptors;

  const ezRTTI* m_pBaseType;
  const ezRTTI* m_pSamplerPinType;
};
