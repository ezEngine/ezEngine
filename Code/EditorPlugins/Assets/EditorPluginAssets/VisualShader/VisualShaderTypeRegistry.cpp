#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_IMPLEMENT_SINGLETON(ezVisualShaderTypeRegistry);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, VisualShader)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualShaderTypeRegistry);

    ezVisualShaderTypeRegistry::GetSingleton()->LoadNodeData();
    const ezRTTI* pBaseType = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtVisualGraphScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezVisualShaderPin>(), [](const ezRTTI* pRtti)->ezQtVisualGraphPin* { return new ezQtVisualShaderPin(); });
    ezQtVisualGraphScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtVisualGraphNode* { return new ezQtVisualShaderNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const ezRTTI* pBaseType = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtVisualGraphScene::GetPinFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualShaderPin>());
    ezQtVisualGraphScene::GetNodeFactory().UnregisterCreator(pBaseType);

    ezVisualShaderTypeRegistry* pDummy = ezVisualShaderTypeRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  static const char* s_szColorNames[] = {
    "Red",
    "Pink",
    "Grape",
    "Violet",
    "Indigo",
    "Blue",
    "Cyan",
    "Teal",
    "Green",
    "Lime",
    "Yellow",
    "Orange",
    "Gray",
  };
  static_assert(EZ_ARRAY_SIZE(s_szColorNames) == ezColorScheme::Count);

  static void GetColorFromDdl(const ezOpenDdlReaderElement* pElement, ezColorGammaUB& out_color)
  {
    if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::String)
    {
      ezColorScheme::Enum color = ezColorScheme::Gray;
      const ezStringView* pValue = pElement->GetPrimitivesString();
      for (ezUInt32 i = 0; i < ezColorScheme::Count; ++i)
      {
        if (pValue->IsEqual_NoCase(s_szColorNames[i]))
        {
          color = static_cast<ezColorScheme::Enum>(i);
          break;
        }
      }

      out_color = ezColorScheme::DarkUI(color);
    }
    else
    {
      ezOpenDdlUtils::ConvertToColorGamma(pElement, out_color).IgnoreResult();
    }
  }
} // namespace

ezVisualShaderTypeRegistry::ezVisualShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{
  m_pBaseType = nullptr;
  m_pSamplerPinType = nullptr;
  ezQtEditorApp::m_Events.AddEventHandler(ezMakeDelegate(&ezVisualShaderTypeRegistry::EditorEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualShaderTypeRegistry::ProjectEventHandler, this));
}

ezVisualShaderTypeRegistry::~ezVisualShaderTypeRegistry()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualShaderTypeRegistry::ProjectEventHandler, this));
  ezQtEditorApp::m_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualShaderTypeRegistry::EditorEventHandler, this));
}

const ezVisualShaderNodeDescriptor* ezVisualShaderTypeRegistry::GetDescriptorForType(const ezRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

void ezVisualShaderTypeRegistry::EditorEventHandler(const ezEditorAppEvent& e)
{
  if (e.m_Type == ezEditorAppEvent::Type::EditorStarted)
  {
    UpdateNodeData();
  }
}

void ezVisualShaderTypeRegistry::ProjectEventHandler(const ezToolsProjectEvent& e)
{
  // The editor startup event fires before any project is loaded, so at that point there are no project
  // data directories to search. They are configured and mounted while ProjectOpened is being handled.
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    LoadProjectNodeData();
  }

  if (e.m_Type == ezToolsProjectEvent::Type::ProjectClosed)
  {
    UnloadProjectNodeData();
  }
}

void ezVisualShaderTypeRegistry::UpdateNodeData()
{
  // If the assets plugin is statically linked, ON_CORESYSTEMS_STARTUP is fired before the editor is running, at which point the data directories are not set up yet so the code below will fail. Therefore, we also run this code in the EditorEventHandler code above to ensure that we run this code at the appropriate time.
  // If linked dynamically, the plugin will be loaded during project open, at which point everything is already running.
  if (!ezQtEditorApp::GetSingleton() || !ezQtEditorApp::GetSingleton()->IsRunning())
    return;

  // the nodes that the editor itself ships
  ezStringBuilder sSearchDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("VisualShader/*.ddl");

  ezFileSystemIterator it;
  for (it.StartSearch(sSearchDir, ezFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    UpdateNodeData(it.GetStats().m_sName);
  }

  // and the nodes of the open project, if there already is one - see ProjectEventHandler()
  LoadProjectNodeData();
}

// Config file paths are stored in the node descriptors and end up as asset transform dependencies.
// Therefore they must never be absolute - make them ':rootname/...' relative to their data directory.
static void MakeConfigFilePathPortable(ezStringBuilder& ref_sPath)
{
  if (!ezPathUtils::IsAbsolutePath(ref_sPath))
    return;

  ezStringBuilder sRelative;
  const ezDataDirectoryInfo* pDataDir = nullptr;

  if (ezFileSystem::ResolvePath(ref_sPath, nullptr, &sRelative, &pDataDir).Failed())
  {
    ezLog::Warning("Visual Shader config file '{}' is not inside a data directory, its path can't be stored in a portable way.", ref_sPath);
    return;
  }

  ref_sPath = sRelative;

  if (!pDataDir->m_sRootName.IsEmpty())
  {
    // the file system stores root names in upper case, but rooted paths are matched case insensitive,
    // so write them in lower case to match the style of all the other paths
    ezStringBuilder sRootName = pDataDir->m_sRootName;
    sRootName.ToLower();

    ref_sPath.Prepend(":", sRootName, "/");
  }
}

void ezVisualShaderTypeRegistry::LoadProjectNodeData()
{
  if (!ezToolsProject::IsProjectOpen())
    return;

  // A project may ship its own nodes in '<data directory>/Editor/VisualShader/*.ddl', e.g. to wrap
  // game specific render states or shader functions, without having to modify the editor's own data.
  // The files are read through the file system, so this must run after the data directories have been
  // applied - which is why this is not part of the editor startup event.
  ezStringBuilder sSearchDir, sNodeFile;
  for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
  {
    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sSearchDir).Failed())
      continue;

    sSearchDir.AppendPath("Editor/VisualShader/*.ddl");

    ezFileSystemIterator it;
    for (it.StartSearch(sSearchDir, ezFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sNodeFile);
      MakeConfigFilePathPortable(sNodeFile);

      LoadConfigFile(sNodeFile, true);
    }
  }
}

void ezVisualShaderTypeRegistry::UnloadProjectNodeData()
{
  for (const ezRTTI* pType : m_ProjectNodeTypes)
  {
    m_NodeDescriptors.Remove(pType);
    ezPhantomRttiManager::UnregisterType(pType);
  }

  m_ProjectNodeTypes.Clear();
}


void ezVisualShaderTypeRegistry::UpdateNodeData(ezStringView sCfgFileRelative)
{
  ezStringBuilder sPath = sCfgFileRelative;
  bool bProjectNode = false;

  if (!ezPathUtils::IsAbsolutePath(sCfgFileRelative))
  {
    sPath.SetFormat(":app/VisualShader/{}", sCfgFileRelative);
  }
  else
  {
    // absolute paths come from the directory watchers, which watch the editor's folder as well as the
    // project's - anything that isn't the editor's own folder belongs to the project
    ezStringBuilder sAppDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
    sAppDir.AppendPath("VisualShader");
    sAppDir.MakeCleanPath();

    sPath.MakeCleanPath();
    bProjectNode = !sPath.StartsWith_NoCase(sAppDir);

    if (bProjectNode)
    {
      MakeConfigFilePathPortable(sPath);
    }
    else
    {
      sPath.MakeRelativeTo(sAppDir).IgnoreResult();
      sPath.Prepend(":app/VisualShader/");
    }
  }

  LoadConfigFile(sPath, bProjectNode);
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
    desc.m_Flags = ezTypeFlags::Abstract | ezTypeFlags::Class;
    desc.m_uiTypeVersion = 1;

    m_pBaseType = ezPhantomRttiManager::RegisterType(desc);
  }

  if (m_pSamplerPinType == nullptr)
  {
    ezReflectedTypeDescriptor desc;
    desc.m_sTypeName = "ezVisualShaderSamplerPin";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
    desc.m_Flags = ezTypeFlags::Class;
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
  desc.m_Flags = ezTypeFlags::Class;
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

void ezVisualShaderTypeRegistry::LoadConfigFile(const char* szFile, bool bProjectNode)
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
    if (ddl.ParseDocument(file, 0, ezLog::GetThreadLocalLogSystem()).Failed())
    {
      ezLog::Error("Failed to parse Visual Shader config file '{0}'", szFile);
      return;
    }

    const ezOpenDdlReaderElement* pRoot = ddl.GetRootElement();
    const ezOpenDdlReaderElement* pNode = pRoot->GetFirstChild();

    while (pNode != nullptr)
    {
      if (!pNode->IsCustomType() || pNode->GetCustomType() != "Node")
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

      const ezRTTI* pType = GenerateTypeFromDesc(nd);
      m_NodeDescriptors.Insert(pType, nd);

      if (bProjectNode && !m_ProjectNodeTypes.Contains(pType))
      {
        m_ProjectNodeTypes.PushBack(pType);
      }

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

  if (pType == ezGetStaticRTTI<bool>())
  {
    bool res = false;
    ezConversionUtils::StringToBool(szDefault, res).IgnoreResult();
    return ezVariant(res);
  }

  float values[4] = {0, 0, 0, 0};
  ezConversionUtils::ExtractFloatsFromString(szDefault, 4, values);

  if (pType == ezGetStaticRTTI<float>())
  {
    return ezVariant(values[0]);
  }

  if (pType == ezGetStaticRTTI<int>())
  {
    return ezVariant((int)values[0]);
  }

  if (pType == ezGetStaticRTTI<ezVec2>())
  {
    return ezVariant(ezVec2(values[0], values[1]));
  }

  if (pType == ezGetStaticRTTI<ezVec3>())
  {
    return ezVariant(ezVec3(values[0], values[1], values[2]));
  }

  if (pType == ezGetStaticRTTI<ezVec4>())
  {
    return ezVariant(ezVec4(values[0], values[1], values[2], values[3]));
  }

  if (pType == ezGetStaticRTTI<ezColor>())
  {
    return ezVariant(ezColorGammaUB(values[0], values[1], values[2], values[3]));
  }

  return ezVariant();
}

void ezVisualShaderTypeRegistry::ExtractNodePins(const ezOpenDdlReaderElement* pNode, const char* szPinType, ezDynamicArray<ezVisualShaderPinDescriptor>& pinArray, bool bOutput)
{
  for (const ezOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == szPinType)
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
        else if (sType == "auto")
          pin.m_pDataType = nullptr; // nullptr indicates "auto" type - computed from inputs at code generation time
        else
        {
          ezLog::Error("Invalid pin type '{0}'", sType);
          continue;
        }
      }

      if (auto pInline = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "Inline"))
      {
        pin.m_sShaderCodeInline = pInline->GetPrimitivesString()[0];
      }
      else if (bOutput)
      {
        ezLog::Error("Output pin '{0}' has no inline code specified", pin.m_sName);
        continue;
      }

      // this is optional
      if (auto pColor = pElement->FindChild("Color"))
      {
        GetColorFromDdl(pColor, pin.m_Color);
      }

      // this is optional
      if (auto pTooltip = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "Tooltip"))
      {
        pin.m_sTooltip = pTooltip->GetPrimitivesString()[0];
      }

      // this is optional
      if (auto pDefaultValue = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "DefaultValue"))
      {
        pin.m_sDefaultValue = pDefaultValue->GetPrimitivesString()[0];
      }

      if (auto pDefineWhenUsingDefaultValue = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "DefineWhenUsingDefaultValue"))
      {
        const ezUInt32 numElements = pDefineWhenUsingDefaultValue->GetNumPrimitives();
        pin.m_sDefinesWhenUsingDefaultValue.Reserve(numElements);

        for (ezUInt32 i = 0; i < numElements; ++i)
        {
          pin.m_sDefinesWhenUsingDefaultValue.PushBack(pDefineWhenUsingDefaultValue->GetPrimitivesString()[i]);
        }
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
        pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::StandardType);

        // For "auto" type pins, use float as the fallback type for the property GUI
        const ezRTTI* pPropertyType = pin.m_pDataType != nullptr ? pin.m_pDataType : ezGetStaticRTTI<float>();
        pin.m_PropertyDesc.m_sType = pPropertyType->GetTypeName();

        const ezVariant def = ExtractDefaultValue(pPropertyType, pin.m_sDefaultValue);

        if (def.IsValid())
        {
          pin.m_PropertyDesc.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezDefaultValueAttribute, def));
        }
      }

      pinArray.PushBack(pin);
    }
  }
}

void ezVisualShaderTypeRegistry::ExtractNodeProperties(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor& nd)
{
  for (const ezOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == "Property")
    {
      ezInt8 iValueGroup = -1;

      ezReflectedPropertyDescriptor prop;
      prop.m_Category = ezPropertyCategory::Member;
      prop.m_Flags.SetValue((ezUInt16)ezPropertyFlags::StandardType);

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

      const ezRTTI* pRtti = nullptr;

      {
        const ezStringView& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
        {
          pRtti = ezGetStaticRTTI<ezColor>();

          // always expose the alpha channel for color properties
          ezExposeColorAlphaAttribute* pAttr = ezExposeColorAlphaAttribute::GetStaticRTTI()->GetAllocator()->Allocate<ezExposeColorAlphaAttribute>();
          prop.m_Attributes.PushBack(pAttr);
        }
        else if (sType == "float4")
        {
          pRtti = ezGetStaticRTTI<ezVec4>();
        }
        else if (sType == "float3")
        {
          pRtti = ezGetStaticRTTI<ezVec3>();
        }
        else if (sType == "float2")
        {
          pRtti = ezGetStaticRTTI<ezVec2>();
        }
        else if (sType == "float")
        {
          pRtti = ezGetStaticRTTI<float>();
        }
        else if (sType == "int")
        {
          pRtti = ezGetStaticRTTI<int>();
        }
        else if (sType == "bool")
        {
          pRtti = ezGetStaticRTTI<bool>();
        }
        else if (sType == "string")
        {
          pRtti = ezGetStaticRTTI<ezString>();
        }
        else if (sType == "identifier")
        {
          pRtti = ezGetStaticRTTI<ezString>();

          iValueGroup = 1; // currently no way to specify the group
        }
        else if (sType == "enum")
        {
          pRtti = ezGetStaticRTTI<ezString>();

          // Read enum values from EnumValues property
          const ezOpenDdlReaderElement* pEnumValues = pElement->FindChildOfType(ezOpenDdlPrimitiveType::String, "EnumValues");
          if (pEnumValues)
          {
            // Create a unique enum name based on the node and property name
            ezStringBuilder sEnumName;
            sEnumName.SetFormat("{}_{}", nd.m_sName, prop.m_sName);

            ezDynamicStringEnumAttribute* pAttr = EZ_DEFAULT_NEW(ezDynamicStringEnumAttribute, sEnumName);
            prop.m_Attributes.PushBack(pAttr);

            // Parse and register the enum values with the dynamic enum registry
            ezStringBuilder enumValuesStr = pEnumValues->GetPrimitivesString()[0];

            // Create or get the dynamic enum
            auto& dynEnum = ezDynamicStringEnum::CreateDynamicEnum(sEnumName);
            dynEnum.Clear();

            // Parse comma-separated values
            ezTempHybridArray<ezStringView, 32> values;
            enumValuesStr.Split(false, values, ",");

            for (const ezStringView& value : values)
            {
              ezStringBuilder trimmedValue = value;
              trimmedValue.Trim(" \t\r\n");
              if (!trimmedValue.IsEmpty())
              {
                dynEnum.AddValidValue(trimmedValue, false);
              }
            }
          }
          else
          {
            ezLog::Error("Property '{}' of type 'enum' is missing 'EnumValues'", prop.m_sName);
            continue;
          }
        }
        else if (sType == "Texture2D")
        {
          pRtti = ezGetStaticRTTI<ezString>();

          // apparently the attributes are deallocated using the type allocator, so we must allocate them here through RTTI as well
          ezAssetBrowserAttribute* pAttr = ezAssetBrowserAttribute::GetStaticRTTI()->GetAllocator()->Allocate<ezAssetBrowserAttribute>();
          pAttr->SetTypeFilter("CompatibleAsset_Texture_2D");
          prop.m_Attributes.PushBack(pAttr);
        }
        else
        {
          ezLog::Error("Invalid property type '{0}'", sType);
          continue;
        }
      }

      prop.m_sType = pRtti->GetTypeName();

      const ezOpenDdlReaderElement* pValue = pElement->FindChild("DefaultValue");
      if (pValue && pRtti != nullptr && pValue->HasPrimitives(ezOpenDdlPrimitiveType::String))
      {
        ezStringBuilder tmp = pValue->GetPrimitivesString()[0];
        const ezVariant def = ExtractDefaultValue(pRtti, tmp);

        if (def.IsValid())
        {
          prop.m_Attributes.PushBack(EZ_DEFAULT_NEW(ezDefaultValueAttribute, def));
        }
      }

      nd.m_Properties.PushBack(prop);
      nd.m_UniquePropertyValueGroups.PushBack(iValueGroup);
    }
  }
}

void ezVisualShaderTypeRegistry::ExtractNodeConfig(const ezOpenDdlReaderElement* pNode, ezVisualShaderNodeDescriptor& nd)
{
  ezStringBuilder temp;

  const ezOpenDdlReaderElement* pElement = pNode->GetFirstChild();

  while (pElement)
  {
    if (pElement->GetName() == "Color")
    {
      GetColorFromDdl(pElement, nd.m_Color);
    }
    else if (pElement->HasPrimitives(ezOpenDdlPrimitiveType::String))
    {
      if (pElement->GetName() == "NodeType")
      {
        if (pElement->GetPrimitivesString()[0] == "Main")
          nd.m_NodeType = ezVisualShaderNodeType::Main;
        else if (pElement->GetPrimitivesString()[0] == "Texture")
          nd.m_NodeType = ezVisualShaderNodeType::Texture;
        else if (pElement->GetPrimitivesString()[0] == "ShaderState")
          nd.m_NodeType = ezVisualShaderNodeType::ShaderState;
        else if (pElement->GetPrimitivesString()[0] == "Parameter")
          nd.m_NodeType = ezVisualShaderNodeType::Parameter;
        else
          nd.m_NodeType = ezVisualShaderNodeType::Generic;
      }
      else if (pElement->GetName() == "Category")
      {
        nd.m_sCategory.Assign(pElement->GetPrimitivesString()[0]);
      }
      else if (pElement->GetName() == "Docs")
      {
        nd.m_sDocs = pElement->GetPrimitivesString()[0];
      }
      else if (pElement->GetName() == "Title")
      {
        nd.m_sTitle = pElement->GetPrimitivesString()[0];
      }
      else if (pElement->GetName() == "CheckPermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        temp.ReplaceAll(" ", "");
        temp.ReplaceAll("\r", "");
        temp.ReplaceAll("\t", "");
        temp.Trim("\n");
        nd.m_sCheckPermutations = temp;
      }
      else if (pElement->GetName() == "CodePermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePermutations = temp;
      }
      else if (pElement->GetName() == "CodeRenderStates")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeRenderState = temp;
      }
      else if (pElement->GetName() == "CodeMaterialConfig")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialConfig = temp;
      }
      else if (pElement->GetName() == "CodeShaderShared")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeShaderShared = temp;
      }
      else if (pElement->GetName() == "CodeVertexDefines")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexDefines = temp;
      }
      else if (pElement->GetName() == "CodeVertexIncludes")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexIncludes = temp;
      }
      else if (pElement->GetName() == "CodeVertexBody")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexBody = temp;
      }
      else if (pElement->GetName() == "CodeMaterialParams")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialParams = temp;
      }
      else if (pElement->GetName() == "CodeMaterialConstants")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialConstants = temp;
      }
      else if (pElement->GetName() == "CodeMaterialCB")
      {
        temp = pElement->GetPrimitivesString()[0];
        nd.m_sShaderCodeMaterialCB = temp;
      }
      else if (pElement->GetName() == "CodePixelDefines")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelDefines = temp;
      }
      else if (pElement->GetName() == "CodePixelIncludes")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelIncludes = temp;
      }
      else if (pElement->GetName() == "CodePixelSamplers")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelSamplers = temp;
      }
      else if (pElement->GetName() == "CodePixelConstants")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelConstants = temp;
      }
      else if (pElement->GetName() == "CodePixelBody")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelBody = temp;
      }
    }

    pElement = pElement->GetSibling();
  }
}
