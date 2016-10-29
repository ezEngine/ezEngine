#include <PCH.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <Foundation/IO/OSFile.h>

EZ_IMPLEMENT_SINGLETON(ezVisualShaderTypeRegistry);

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualShader)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "PluginAssets"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualShaderTypeRegistry);

    ezVisualShaderTypeRegistry::GetSingleton()->LoadNodeData();
    const ezRTTI* pBaseType = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezVisualShaderPin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtVisualShaderPin(); });
    ezQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtVisualShaderNode(); });
  }

  ON_CORE_SHUTDOWN
  {
    ezVisualShaderTypeRegistry* pDummy = ezVisualShaderTypeRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION

ezVisualShaderTypeRegistry::ezVisualShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{
    m_pBaseType = nullptr;
}

const ezVisualShaderNodeDescriptor* ezVisualShaderTypeRegistry::GetDescriptorForType(const ezRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}


void ezVisualShaderTypeRegistry::UpdateNodeData()
{
  ezStringBuilder sSearchDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("VisualShader/*.json");

  ezFileSystemIterator it;
  if (it.StartSearch(sSearchDir, false, false).Succeeded())
  {
    do
    {
      UpdateNodeData(it.GetStats().m_sFileName);
    }
    while (it.Next().Succeeded());
  }
}


void ezVisualShaderTypeRegistry::UpdateNodeData(const char* szCfgFileRelative)
{
  ezStringBuilder sPath = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sPath.AppendPath("VisualShader", szCfgFileRelative);

  LoadConfigFile(sPath);
}

void ezVisualShaderTypeRegistry::LoadNodeData()
{
  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    ezReflectedTypeDescriptor desc;
    desc.m_sTypeName = "ezVisualShaderNodeBase";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
    desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Abstract | ezTypeFlags::Class;
    desc.m_uiTypeSize = 0;
    desc.m_uiTypeVersion = 1;

    m_pBaseType = ezPhantomRttiManager::RegisterType(desc);
  }

  UpdateNodeData();
}

const ezRTTI* ezVisualShaderTypeRegistry::GenerateTypeFromDesc(const ezVisualShaderNodeDescriptor& nd) const
{
  ezStringBuilder temp;

  temp.Set("ShaderNode::", nd.m_sName);

  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = temp;
  desc.m_sPluginName = "VisualShaderTypes";
  desc.m_sParentTypeName = "ezVisualShaderNodeBase";
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;
  desc.m_uiTypeSize = 0;
  desc.m_uiTypeVersion = 1;
  desc.m_Properties = nd.m_Properties;

  for (const auto& pin : nd.m_InputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  for (const auto& pin : nd.m_OutputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  return ezPhantomRttiManager::RegisterType(desc);
}

void ezVisualShaderTypeRegistry::LoadConfigFile(const char* szFile)
{
  EZ_LOG_BLOCK("Loading Visual Shader Config", szFile);

  ezLog::Debug("Loading VSE node config '%s'", szFile);

  ezFileReader file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open Visual Shader config file '%s'", szFile);
    return;
  }

  ezJSONReader json;
  json.SetLogInterface(ezGlobalLog::GetOrCreateInstance());

  if (json.Parse(file).Failed())
  {
    ezLog::Error("Failed to parse Visual Shader config file '%s'", szFile);
    return;
  }

  const ezVariantDictionary& top = json.GetTopLevelObject();

  for (auto itTop = top.GetIterator(); itTop.IsValid(); ++itTop)
  {
    const ezVariant& varNode = itTop.Value();
    if (!varNode.IsA<ezVariantDictionary>())
    {
      ezLog::Error("Block '%s' is not a dictionary", itTop.Key().GetData());
      continue;
    }

    const ezVariantDictionary& varNodeDict = varNode.Get<ezVariantDictionary>();

    ezVisualShaderNodeDescriptor nd;
    nd.m_sName = itTop.Key();

    ExtractNodeConfig(varNodeDict, nd);
    ExtractNodeProperties(varNodeDict, nd);
    ExtractNodePins(varNodeDict, "InputPins", nd.m_InputPins);
    ExtractNodePins(varNodeDict, "OutputPins", nd.m_OutputPins);

    m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
  }
}


void ezVisualShaderTypeRegistry::ExtractNodePins(const ezVariantDictionary &varNodeDict, const char* szPinType, ezHybridArray<ezVisualShaderPinDescriptor, 4> &pinArray)
{
  ezVariant varValue;
  if (varNodeDict.TryGetValue(szPinType, varValue) && varValue.IsA<ezVariantArray>())
  {
    const ezVariantArray& varPins = varValue.Get<ezVariantArray>();

    for (ezUInt32 p = 0; p < varPins.GetCount(); ++p)
    {
      if (!varPins[p].IsA<ezVariantDictionary>())
      {
        ezLog::Error("Properties array contains values that are not dictionaries");
        continue;
      }

      const ezVariantDictionary& varPin = varPins[p].Get<ezVariantDictionary>();
      ezVisualShaderPinDescriptor pin;

      if (!varPin.TryGetValue("Name", varValue) || !varValue.IsA<ezString>())
      {
        ezLog::Error("Missing or invalid name for pin");
        continue;
      }

      pin.m_sName = varValue.Get<ezString>();

      if (!varPin.TryGetValue("Type", varValue) || !varValue.IsA<ezString>())
      {
        ezLog::Error("Missing or invalid pin type");
        continue;
      }

      {
        const ezString& sType = varValue.Get<ezString>();

        if (sType == "color")
          pin.m_pDataType = ezGetStaticRTTI<ezColor>();
        else if (sType == "float4")
          pin.m_pDataType = ezGetStaticRTTI<ezVec4>();
        else if (sType == "float3")
          pin.m_pDataType = ezGetStaticRTTI<ezVec3>();
        else if (sType == "float2")
          pin.m_pDataType = ezGetStaticRTTI<ezVec2>();
        else if (sType == "float")
          pin.m_pDataType = ezGetStaticRTTI<float>();
        else
        {
          ezLog::Error("Invalid pin type '%s'", sType.GetData());
          continue;
        }
      }

      /// \todo Enforce this for output pins
      if (varPin.TryGetValue("Inline", varValue) && varValue.IsA<ezString>())
      {
        pin.m_sShaderCodeInline = varValue.Get<ezString>();
      }

      // this is optional
      if (varPin.TryGetValue("Color", varValue) && varValue.IsA<ezString>())
      {
        float col[3] = { 255, 255, 255 };
        ezConversionUtils::ExtractFloatsFromString(varValue.Get<ezString>(), 3, col);
        pin.m_Color = ezColorGammaUB((ezUInt8)col[0], (ezUInt8)col[1], (ezUInt8)col[2]);
      }

      // this is optional
      if (varPin.TryGetValue("Tooltip", varValue) && varValue.IsA<ezString>())
      {
        pin.m_sTooltip = varValue.Get<ezString>();
      }

      // this is optional
      if (varPin.TryGetValue("Expose", varValue) && varValue.IsA<bool>())
      {
        pin.m_bExposeAsProperty = varValue.Get<bool>();
      }

      // this is optional
      if (varPin.TryGetValue("Fallback", varValue) && varValue.IsA<ezString>())
      {
        /// \todo Keep this a variant?
        pin.m_DefaultValue = varValue.Get<ezString>();
      }

      if (pin.m_bExposeAsProperty)
      {
        pin.m_PropertyDesc.m_sName = pin.m_sName;
        pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
        pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
        pin.m_PropertyDesc.m_sType = pin.m_pDataType->GetTypeName();
      }

      pinArray.PushBack(pin);
    }
  }
}

void ezVisualShaderTypeRegistry::ExtractNodeProperties(const ezVariantDictionary &varNodeDict, ezVisualShaderNodeDescriptor &nd)
{
  ezVariant varValue;
  if (varNodeDict.TryGetValue("Properties", varValue) && varValue.IsA<ezVariantArray>())
  {
    const ezVariantArray& varProperties = varValue.Get<ezVariantArray>();

    for (ezUInt32 p = 0; p < varProperties.GetCount(); ++p)
    {
      if (!varProperties[p].IsA<ezVariantDictionary>())
      {
        ezLog::Error("Properties array contains values that are not dictionaries");
        continue;
      }

      const ezVariantDictionary& varProperty = varProperties[p].Get<ezVariantDictionary>();

      ezReflectedPropertyDescriptor prop;
      prop.m_Category = ezPropertyCategory::Member;
      prop.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);

      if (!varProperty.TryGetValue("Name", varValue) || !varValue.IsA<ezString>())
      {
        ezLog::Error("Property doesn't have a name");
        continue;
      }

      prop.m_sName = varValue.Get<ezString>();

      if (!varProperty.TryGetValue("Type", varValue) || !varValue.IsA<ezString>())
      {
        ezLog::Error("Property doesn't have a type");
        continue;
      }

      {
        const ezString& sType = varValue.Get<ezString>();

        if (sType == "color")
          prop.m_sType = ezGetStaticRTTI<ezColor>()->GetTypeName();
        else if (sType == "float4")
          prop.m_sType = ezGetStaticRTTI<ezVec4>()->GetTypeName();
        else if (sType == "float3")
          prop.m_sType = ezGetStaticRTTI<ezVec3>()->GetTypeName();
        else if (sType == "float2")
          prop.m_sType = ezGetStaticRTTI<ezVec2>()->GetTypeName();
        else if (sType == "float")
          prop.m_sType = ezGetStaticRTTI<float>()->GetTypeName();
        else
        {
          ezLog::Error("Invalid property type '%s'", sType.GetData());
          continue;
        }
      }

      if (varProperty.TryGetValue("Value", varValue) && varValue.IsA<ezString>())
      {
        // TODO Default property value

      }

      nd.m_Properties.PushBack(prop);
    }
  }
}

void ezVisualShaderTypeRegistry::ExtractNodeConfig(const ezVariantDictionary &varNodeDict, ezVisualShaderNodeDescriptor& nd)
{
  ezVariant varValue;
  if (varNodeDict.TryGetValue("Color", varValue) && varValue.IsA<ezString>())
  {
    float col[3] = { 255, 255, 255 };
    ezConversionUtils::ExtractFloatsFromString(varValue.Get<ezString>(), 3, col);
    nd.m_Color = ezColorGammaUB((ezUInt8)col[0], (ezUInt8)col[1], (ezUInt8)col[2]);
  }

  if (varNodeDict.TryGetValue("NodeType", varValue) && varValue.IsA<ezString>())
  {
    if (varValue.Get<ezString>() == "Main")
      nd.m_NodeType = ezVisualShaderNodeType::Main;
    else
      nd.m_NodeType = ezVisualShaderNodeType::Generic;
  }

  ezStringBuilder temp;

  if (varNodeDict.TryGetValue("CodePermutations", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodePermutations = temp;
  }

  if (varNodeDict.TryGetValue("CodeRenderStates", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodeRenderState = temp;
  }

  if (varNodeDict.TryGetValue("CodeVertexShader", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodeVertexShader = temp;
  }

  if (varNodeDict.TryGetValue("CodeMaterialParams", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodeMaterialParams = temp;
  }

  if (varNodeDict.TryGetValue("CodePixelDefines", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodePixelDefines = temp;
  }

  if (varNodeDict.TryGetValue("CodePixelIncludes", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodePixelIncludes = temp;
  }

  if (varNodeDict.TryGetValue("CodePixelSamplers", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodePixelSamplers = temp;
  }

  if (varNodeDict.TryGetValue("CodePixelConstants", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodePixelConstants = temp;
  }

  if (varNodeDict.TryGetValue("CodePixelBody", varValue) && varValue.IsA<ezString>())
  {
    temp = varValue.Get<ezString>();
    if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
    nd.m_sShaderCodePixelBody = temp;
  }
}

