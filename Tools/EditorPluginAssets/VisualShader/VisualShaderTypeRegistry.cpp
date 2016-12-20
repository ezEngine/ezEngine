#include <PCH.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>

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
  ezQtNodeScene::GetConnectionFactory().RegisterCreator(ezGetStaticRTTI<ezVisualShaderConnection>(), [](const ezRTTI* pRtti)->ezQtConnection* { return new ezQtVisualShaderConnection(); });
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
  m_pSamplerPinType = nullptr;
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
  sSearchDir.AppendPath("VisualShader/*.ddl");

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
  ezStringBuilder sPath(":app/VisualShader/", szCfgFileRelative);

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

  if (m_pSamplerPinType == nullptr)
  {
    ezReflectedTypeDescriptor desc;
    desc.m_sTypeName = "ezVisualShaderSamplerPin";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
    desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;
    desc.m_uiTypeSize = 0;
    desc.m_uiTypeVersion = 1;

    m_pSamplerPinType = ezPhantomRttiManager::RegisterType(desc);
  }

  UpdateNodeData();
}

const ezRTTI* ezVisualShaderTypeRegistry::GenerateTypeFromDesc(const ezVisualShaderNodeDescriptor& nd)
{
  ezStringBuilder temp;
  temp.Set("ShaderNode::", nd.m_sName);

  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = temp;
  desc.m_sPluginName = "VisualShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
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

  ezLog::Debug("Loading VSE node config '{0}'", szFile);

  ezFileReader file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open Visual Shader config file '{0}'", szFile);
    return;
  }

  if (ezPathUtils::HasExtension(szFile, "ddl"))
  {
    ezOpenDdlReader ddl;
    if (ddl.ParseDocument(file, 0, ezGlobalLog::GetOrCreateInstance()).Failed())
    {
      ezLog::Error("Failed to parse Visual Shader config file '{0}'", szFile);
      return;
    }

    const ezOpenDdlReaderElement* pRoot = ddl.GetRootElement();
    const ezOpenDdlReaderElement* pNode = pRoot->GetFirstChild();

    while (pNode != nullptr)
    {
      if (!pNode->IsCustomType() || !ezStringUtils::IsEqual(pNode->GetCustomType(), "Node"))
      {
        ezLog::Error("Top-Level object is not a 'Node' type");
        continue;
      }

      ezVisualShaderNodeDescriptor nd;
      nd.m_sCfgFile = szFile;
      nd.m_sName = pNode->GetName();

      ExtractNodeConfig(pNode, nd);
      ExtractNodeProperties(pNode, nd);
      ExtractNodePins(pNode, "InputPin", nd.m_InputPins, false);
      ExtractNodePins(pNode, "OutputPin", nd.m_OutputPins, true);

      m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);

      pNode = pNode->GetSibling();
    }
  }
}

static ezVariant ExtractDefaultValue(const ezRTTI* pType, const char* szDefault)
{
  if (pType == ezGetStaticRTTI<ezString>())
  {
    return ezVariant(szDefault);
  }

  float values[4] = { 0, 0, 0, 0 };
  ezConversionUtils::ExtractFloatsFromString(szDefault, 4, values);

  if (pType == ezGetStaticRTTI<float>())
  {
    return ezVariant(values[0]);
  }

  if (pType == ezGetStaticRTTI<ezVec2>())
  {
    return ezVariant(ezVec2(values[0], values[1]));
  }

  if (pType == ezGetStaticRTTI<ezVec3>())
  {
    return ezVariant(ezVec3(values[0], values[1], values[2]));
  }

  if (pType == ezGetStaticRTTI<ezVec4>() || pType == ezGetStaticRTTI<ezColor>())
  {
    return ezVariant(ezVec4(values[0], values[1], values[2], values[3]));
  }

  return ezVariant();
}

void ezVisualShaderTypeRegistry::ExtractNodePins(const ezVariantDictionary &varNodeDict, const char* szPinType, ezHybridArray<ezVisualShaderPinDescriptor, 4> &pinArray, bool bOutput)
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
        else if (sType == "string")
          pin.m_pDataType = ezGetStaticRTTI<ezString>();
        else if (sType == "sampler")
          pin.m_pDataType = m_pSamplerPinType;
        else
        {
          ezLog::Error("Invalid pin type '{0}'", sType.GetData());
          continue;
        }
      }

      /// \todo Enforce this for output pins
      if (varPin.TryGetValue("Inline", varValue) && varValue.IsA<ezString>())
      {
        pin.m_sShaderCodeInline = varValue.Get<ezString>();
      }
      else if (bOutput)
      {
        ezLog::Error("Output pin '{0}' has no inline code specified", pin.m_sName.GetData());
        continue;
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
      if (varPin.TryGetValue("Fallback", varValue) && varValue.IsA<ezString>())
      {
        pin.m_sDefaultValue = varValue.Get<ezString>();
      }

      // this is optional
      if (varPin.TryGetValue("Expose", varValue) && varValue.IsA<bool>())
      {
        pin.m_bExposeAsProperty = varValue.Get<bool>();
      }

      if (pin.m_bExposeAsProperty)
      {
        pin.m_PropertyDesc.m_sName = pin.m_sName;
        pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
        pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
        pin.m_PropertyDesc.m_sType = pin.m_pDataType->GetTypeName();

        const ezVariant def = ExtractDefaultValue(pin.m_pDataType, pin.m_sDefaultValue);

        if (def.IsValid())
        {
          pin.m_PropertyDesc.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezDefaultValueAttribute, def));
        }
      }

      pinArray.PushBack(pin);
    }
  }
}


void ezVisualShaderTypeRegistry::ExtractNodePins(const ezOpenDdlReaderElement* pNode, const char* szPinType, ezHybridArray<ezVisualShaderPinDescriptor, 4> &pinArray, bool bOutput)
{
  const ezOpenDdlReaderElement* pElement = pNode->GetFirstChild();

  while (pElement)
  {
    if (ezStringUtils::IsEqual(pElement->GetCustomType(), szPinType))
    {
      ezVisualShaderPinDescriptor pin;

      if (!pElement->HasName())
      {
        ezLog::Error("Missing or invalid name for pin");
        continue;
      }

      pin.m_sName = pElement->GetName();

      auto pType = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "Type");

      if (!pType)
      {
        ezLog::Error("Missing or invalid pin type");
        continue;
      }

      {
        const ezString& sType = pType->GetPrimitivesString()[0];

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
        else if (sType == "string")
          pin.m_pDataType = ezGetStaticRTTI<ezString>();
        else if (sType == "sampler")
          pin.m_pDataType = m_pSamplerPinType;
        else
        {
          ezLog::Error("Invalid pin type '{0}'", sType.GetData());
          continue;
        }
      }

      if (auto pInline = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "Inline"))
      {
        pin.m_sShaderCodeInline = pInline->GetPrimitivesString()[0];
      }
      else if (bOutput)
      {
        ezLog::Error("Output pin '{0}' has no inline code specified", pin.m_sName.GetData());
        continue;
      }

      // this is optional
      if (auto pColor = pElement->FindChild("Color"))
      {
        ezOpenDdlUtils::ConvertToColorGamma(pColor, pin.m_Color);
      }

      // this is optional
      if (auto pTooltip = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "Tooltip"))
      {
        pin.m_sTooltip = pTooltip->GetPrimitivesString()[0];
      }

      // this is optional
      if (auto pFallback = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "Fallback"))
      {
        pin.m_sDefaultValue = pFallback->GetPrimitivesString()[0];
      }

      // this is optional
      if (auto pExpose = pElement->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "Expose"))
      {
        pin.m_bExposeAsProperty = pExpose->GetPrimitivesBool()[0];
      }

      if (pin.m_bExposeAsProperty)
      {
        pin.m_PropertyDesc.m_sName = pin.m_sName;
        pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
        pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
        pin.m_PropertyDesc.m_sType = pin.m_pDataType->GetTypeName();

        const ezVariant def = ExtractDefaultValue(pin.m_pDataType, pin.m_sDefaultValue);

        if (def.IsValid())
        {
          pin.m_PropertyDesc.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezDefaultValueAttribute, def));
        }
      }

      pinArray.PushBack(pin);
    }


    pElement = pElement->GetSibling();
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
        {
          prop.m_sType = ezGetStaticRTTI<ezColor>()->GetTypeName();

          // always expose the alpha channel for color properties
          ezExposeColorAlphaAttribute* pAttr = static_cast<ezExposeColorAlphaAttribute*>(ezExposeColorAlphaAttribute::GetStaticRTTI()->GetAllocator()->Allocate());
          prop.m_Attributes.PushBack(pAttr);
        }
        else if (sType == "float4")
          prop.m_sType = ezGetStaticRTTI<ezVec4>()->GetTypeName();
        else if (sType == "float3")
          prop.m_sType = ezGetStaticRTTI<ezVec3>()->GetTypeName();
        else if (sType == "float2")
          prop.m_sType = ezGetStaticRTTI<ezVec2>()->GetTypeName();
        else if (sType == "float")
          prop.m_sType = ezGetStaticRTTI<float>()->GetTypeName();
        else if (sType == "string")
          prop.m_sType = ezGetStaticRTTI<ezString>()->GetTypeName();
        else if (sType == "Texture2D")
        {
          prop.m_sType = ezGetStaticRTTI<ezString>()->GetTypeName();

          // apparently the attributes are deallocated using the type allocator, so we must allocate them here through RTTI as well
          ezAssetBrowserAttribute* pAttr = static_cast<ezAssetBrowserAttribute*>(ezAssetBrowserAttribute::GetStaticRTTI()->GetAllocator()->Allocate());
          pAttr->SetTypeFilter("Texture 2D");
          prop.m_Attributes.PushBack(pAttr);
        }
        else
        {
          ezLog::Error("Invalid property type '{0}'", sType.GetData());
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


void ezVisualShaderTypeRegistry::ExtractNodeProperties(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor &nd)
{
  const ezOpenDdlReaderElement* pElement = pNode->GetFirstChild();

  while (pElement)
  {
    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Property"))
    {
      ezReflectedPropertyDescriptor prop;
      prop.m_Category = ezPropertyCategory::Member;
      prop.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);

      if (!pElement->HasName())
      {
        ezLog::Error("Property doesn't have a name");
        continue;
      }

      prop.m_sName = pElement->GetName();

      const ezOpenDdlReaderElement* pType = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "Type");
      if (!pType)
      {
        ezLog::Error("Property doesn't have a type");
        continue;
      }

      {
        const ezStringView& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
        {
          prop.m_sType = ezGetStaticRTTI<ezColor>()->GetTypeName();

          // always expose the alpha channel for color properties
          ezExposeColorAlphaAttribute* pAttr = static_cast<ezExposeColorAlphaAttribute*>(ezExposeColorAlphaAttribute::GetStaticRTTI()->GetAllocator()->Allocate());
          prop.m_Attributes.PushBack(pAttr);
        }
        else if (sType == "float4")
          prop.m_sType = ezGetStaticRTTI<ezVec4>()->GetTypeName();
        else if (sType == "float3")
          prop.m_sType = ezGetStaticRTTI<ezVec3>()->GetTypeName();
        else if (sType == "float2")
          prop.m_sType = ezGetStaticRTTI<ezVec2>()->GetTypeName();
        else if (sType == "float")
          prop.m_sType = ezGetStaticRTTI<float>()->GetTypeName();
        else if (sType == "string")
          prop.m_sType = ezGetStaticRTTI<ezString>()->GetTypeName();
        else if (sType == "Texture2D")
        {
          prop.m_sType = ezGetStaticRTTI<ezString>()->GetTypeName();

          // apparently the attributes are deallocated using the type allocator, so we must allocate them here through RTTI as well
          ezAssetBrowserAttribute* pAttr = static_cast<ezAssetBrowserAttribute*>(ezAssetBrowserAttribute::GetStaticRTTI()->GetAllocator()->Allocate());
          pAttr->SetTypeFilter("Texture 2D");
          prop.m_Attributes.PushBack(pAttr);
        }
        else
        {
          ezLog::Error("Invalid property type '{0}'", sType.GetData());
          continue;
        }
      }

      const ezOpenDdlReaderElement* pValue = pElement->FindChild("Value");
      if (pValue)
      {
        // TODO Default property value (could be any type)

      }

      nd.m_Properties.PushBack(prop);
    }

    pElement = pElement->GetSibling();
  }
}

void ezVisualShaderTypeRegistry::ExtractNodeConfig(const ezVariantDictionary &varNodeDict, ezVisualShaderNodeDescriptor& nd)
{
  ezStringBuilder temp;

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
    else if (varValue.Get<ezString>() == "Texture")
      nd.m_NodeType = ezVisualShaderNodeType::Texture;
    else
      nd.m_NodeType = ezVisualShaderNodeType::Generic;
  }

  if (varNodeDict.TryGetValue("Category", varValue) && varValue.IsA<ezString>())
  {
    nd.m_sCategory = varValue.Get<ezString>();
  }

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

void ezVisualShaderTypeRegistry::ExtractNodeConfig(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor &nd)
{
  ezStringBuilder temp;

  const ezOpenDdlReaderElement* pElement = pNode->GetFirstChild();

  while (pElement)
  {
    if (ezStringUtils::IsEqual(pElement->GetName(), "Color"))
    {
      ezOpenDdlUtils::ConvertToColorGamma(pElement, nd.m_Color);
    }
    else if (pElement->HasPrimitives(ezOpenDdlPrimitiveType::String))
    {
      if (ezStringUtils::IsEqual(pElement->GetName(), "NodeType"))
      {
        if (pElement->GetPrimitivesString()[0] == "Main")
          nd.m_NodeType = ezVisualShaderNodeType::Main;
        else if (pElement->GetPrimitivesString()[0] == "Texture")
          nd.m_NodeType = ezVisualShaderNodeType::Texture;
        else
          nd.m_NodeType = ezVisualShaderNodeType::Generic;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "Category"))
      {
        nd.m_sCategory = pElement->GetPrimitivesString()[0];
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodePermutations"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodePermutations = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodeRenderStates"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodeRenderState = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodeVertexShader"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodeVertexShader = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodeMaterialParams"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodeMaterialParams = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodePixelDefines"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodePixelDefines = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodePixelIncludes"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodePixelIncludes = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodePixelSamplers"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodePixelSamplers = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodePixelConstants"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodePixelConstants = temp;
      }
      else if (ezStringUtils::IsEqual(pElement->GetName(), "CodePixelBody"))
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n")) temp.Append("\n");
        nd.m_sShaderCodePixelBody = temp;
      }
    }

    pElement = pElement->GetSibling();
  }
}
