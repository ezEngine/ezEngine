#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

EZ_IMPLEMENT_SINGLETON(ezShaderTypeRegistry);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, ShaderTypeRegistry)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "ReflectedTypeManager"
END_SUBSYSTEM_DEPENDENCIES

ON_CORESYSTEMS_STARTUP
{
  EZ_DEFAULT_NEW(ezShaderTypeRegistry);
}

ON_CORESYSTEMS_SHUTDOWN
{
  ezShaderTypeRegistry* pDummy = ezShaderTypeRegistry::GetSingleton();
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
  struct PermutationVarConfig
  {
    ezVariant m_DefaultValue;
    const ezRTTI* m_pType;
  };

  static ezHashTable<ezString, PermutationVarConfig> s_PermutationVarConfigs;
  static ezHashTable<ezString, const ezRTTI*> s_EnumTypes;

  const ezRTTI* GetPermutationType(const ezShaderParser::ParameterDefinition& def)
  {
    EZ_ASSERT_DEV(def.m_sType.IsEqual("Permutation"), "");

    PermutationVarConfig* pConfig = nullptr;
    if (s_PermutationVarConfigs.TryGetValue(def.m_sName, pConfig))
    {
      return pConfig->m_pType;
    }

    ezStringBuilder sTemp;
    sTemp.SetFormat("Shaders/PermutationVars/{0}.ezPermVar", def.m_sName);

    ezString sPath = sTemp;
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);

    ezFileReader file;
    if (file.Open(sPath).Failed())
    {
      return nullptr;
    }

    sTemp.ReadAll(file);

    ezVariant defaultValue;
    ezShaderParser::EnumDefinition enumDefinition;

    ezShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDefinition);
    if (defaultValue.IsValid())
    {
      pConfig = &(s_PermutationVarConfigs[def.m_sName]);
      pConfig->m_DefaultValue = defaultValue;

      if (defaultValue.IsA<bool>())
      {
        pConfig->m_pType = ezGetStaticRTTI<bool>();
      }
      else
      {
        ezReflectedTypeDescriptor descEnum;
        descEnum.m_sTypeName = def.m_sName;
        descEnum.m_sPluginName = "ShaderTypes";
        descEnum.m_sParentTypeName = ezGetStaticRTTI<ezEnumBase>()->GetTypeName();
        descEnum.m_Flags = ezTypeFlags::IsEnum | ezTypeFlags::Phantom;
        descEnum.m_uiTypeVersion = 1;

        ezArrayPtr<ezPropertyAttribute* const> noAttributes;

        ezStringBuilder sEnumName;
        sEnumName.SetFormat("{0}::Default", def.m_sName);

        descEnum.m_Properties.PushBack(ezReflectedPropertyDescriptor(sEnumName, defaultValue.Get<ezUInt32>(), noAttributes));

        for (const auto& ev : enumDefinition.m_Values)
        {
          ezStringBuilder sEnumName;
          sEnumName.SetFormat("{0}::{1}", def.m_sName, ev.m_sValueName);

          descEnum.m_Properties.PushBack(ezReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
        }

        pConfig->m_pType = ezPhantomRttiManager::RegisterType(descEnum);
      }

      return pConfig->m_pType;
    }

    return nullptr;
  }

  const ezRTTI* GetEnumType(const ezShaderParser::EnumDefinition& def)
  {
    const ezRTTI* pType = nullptr;
    if (s_EnumTypes.TryGetValue(def.m_sName, pType))
    {
      return pType;
    }

    ezReflectedTypeDescriptor descEnum;
    descEnum.m_sTypeName = def.m_sName;
    descEnum.m_sPluginName = "ShaderTypes";
    descEnum.m_sParentTypeName = ezGetStaticRTTI<ezEnumBase>()->GetTypeName();
    descEnum.m_Flags = ezTypeFlags::IsEnum | ezTypeFlags::Phantom;
    descEnum.m_uiTypeVersion = 1;

    ezArrayPtr<ezPropertyAttribute* const> noAttributes;

    ezStringBuilder sEnumName;
    sEnumName.SetFormat("{0}::Default", def.m_sName);

    descEnum.m_Properties.PushBack(ezReflectedPropertyDescriptor(sEnumName, def.m_uiDefaultValue, noAttributes));

    for (const auto& ev : def.m_Values)
    {
      ezStringBuilder sEnumName;
      sEnumName.SetFormat("{0}::{1}", def.m_sName, ev.m_sValueName);

      descEnum.m_Properties.PushBack(ezReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
    }

    pType = ezPhantomRttiManager::RegisterType(descEnum);

    s_EnumTypes.Insert(def.m_sName, pType);

    return pType;
  }

  const ezRTTI* GetType(const ezShaderParser::ParameterDefinition& def)
  {
    if (def.m_pType != nullptr)
    {
      return def.m_pType;
    }

    if (def.m_sType.IsEqual("Permutation"))
    {
      return GetPermutationType(def);
    }

    const ezRTTI* pType = nullptr;
    s_EnumTypes.TryGetValue(def.m_sType, pType);

    return pType;
  }

  void AddAttributes(ezShaderParser::ParameterDefinition& ref_def, const ezRTTI* pType, ezHybridArray<const ezPropertyAttribute*, 2>& ref_attributes)
  {
    if (ref_def.m_sType.StartsWith_NoCase("texture"))
    {
      if (ref_def.m_sType.IsEqual("Texture2D"))
      {
        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Texture 2D"));
        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezAssetBrowserAttribute, "CompatibleAsset_Texture_2D"));
      }
      else if (ref_def.m_sType.IsEqual("Texture3D"))
      {
        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Texture 3D"));
        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezAssetBrowserAttribute, "CompatibleAsset_Texture_3D"));
      }
      else if (ref_def.m_sType.IsEqual("TextureCube"))
      {
        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Texture Cube"));
        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezAssetBrowserAttribute, "CompatibleAsset_Texture_Cube"));
      }
    }
    else if (ref_def.m_sType.StartsWith_NoCase("permutation"))
    {
      ref_attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Permutation"));
    }
    else
    {
      ref_attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Constant"));
    }

    for (auto& attributeDef : ref_def.m_Attributes)
    {
      if (attributeDef.m_sType.IsEqual("Default") && attributeDef.m_Values.GetCount() >= 1)
      {
        if (pType == ezGetStaticRTTI<ezColor>())
        {
          // always expose the alpha channel for color properties
          ref_attributes.PushBack(EZ_DEFAULT_NEW(ezExposeColorAlphaAttribute));

          // patch default type, VSE writes float4 instead of color
          if (attributeDef.m_Values[0].GetType() == ezVariantType::Vector4)
          {
            ezVec4 v = attributeDef.m_Values[0].Get<ezVec4>();
            attributeDef.m_Values[0] = ezColor(v.x, v.y, v.z, v.w);
          }
        }

        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezDefaultValueAttribute, attributeDef.m_Values[0]));
      }
      else if (attributeDef.m_sType.IsEqual("Clamp") && attributeDef.m_Values.GetCount() >= 2)
      {
        ref_attributes.PushBack(EZ_DEFAULT_NEW(ezClampValueAttribute, attributeDef.m_Values[0], attributeDef.m_Values[1]));
      }
      else if (attributeDef.m_sType.IsEqual("Group"))
      {
        if (attributeDef.m_Values.GetCount() >= 1 && attributeDef.m_Values[0].CanConvertTo<ezString>())
        {
          ref_attributes.PushBack(EZ_DEFAULT_NEW(ezGroupAttribute, attributeDef.m_Values[0].ConvertTo<ezString>()));
        }
        else
        {
          ref_attributes.PushBack(EZ_DEFAULT_NEW(ezGroupAttribute));
        }
      }
    }
  }
} // namespace

ezShaderTypeRegistry::ezShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{
  ezShaderTypeRegistry::GetSingleton();
  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = "ezShaderTypeBase";
  desc.m_sPluginName = "ShaderTypes";
  desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Abstract | ezTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  m_pBaseType = ezPhantomRttiManager::RegisterType(desc);

  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


ezShaderTypeRegistry::~ezShaderTypeRegistry()
{
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const ezRTTI* ezShaderTypeRegistry::GetShaderType(ezStringView sShaderPath0)
{
  if (sShaderPath0.IsEmpty())
    return nullptr;

  ezStringBuilder sShaderPath = sShaderPath0;
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsAbsolutePath())
  {
    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sShaderPath))
    {
      ezLog::Error("Could not make shader path '{0}' relative!", sShaderPath);
    }
  }

  auto it = m_ShaderTypes.Find(sShaderPath);
  if (it.IsValid())
  {
    ezFileStats Stats;
    if (ezOSFile::GetFileStats(it.Value().m_sAbsShaderPath, Stats).Succeeded() &&
        !Stats.m_LastModificationTime.Compare(it.Value().m_fileModifiedTime, ezTimestamp::CompareMode::FileTimeEqual))
    {
      UpdateShaderType(it.Value());
    }
  }
  else
  {
    ezStringBuilder sAbsPath = sShaderPath0;
    {
      if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
      {
        ezLog::Warning("Can't make path absolute: '{0}'", sShaderPath0);
        return nullptr;
      }
      sAbsPath.MakeCleanPath();
    }

    it = m_ShaderTypes.Insert(sShaderPath, ShaderData());
    it.Value().m_sShaderPath = sShaderPath;
    it.Value().m_sAbsShaderPath = sAbsPath;
    UpdateShaderType(it.Value());
  }

  return it.Value().m_pType;
}

void ezShaderTypeRegistry::UpdateShaderType(ShaderData& data)
{
  EZ_LOG_BLOCK("Updating Shader Parameters", data.m_sShaderPath.GetData());

  ezHybridArray<ezShaderParser::ParameterDefinition, 16> parameters;
  ezHybridArray<ezShaderParser::EnumDefinition, 4> enumDefinitions;

  {
    ezFileStats Stats;
    bool bStat = ezOSFile::GetFileStats(data.m_sAbsShaderPath, Stats).Succeeded();

    ezFileReader file;
    if (!bStat || file.Open(data.m_sAbsShaderPath).Failed())
    {
      ezLog::Error("Can't update shader '{0}' type information, the file can't be opened.", data.m_sShaderPath);
      return;
    }

    ezString sContent;
    sContent.ReadAll(file);
    ezShaderHelper::ezTextSectionizer sections;
    ezShaderHelper::GetShaderSections(sContent, sections);
    ezUInt32 uiFirstLine = 0;
    ezStringView sSectionContent = sections.GetSectionContent(ezShaderHelper::ezShaderSections::MATERIALPARAMETER, uiFirstLine);

    ezShaderParser::ParseMaterialParameterSection(sSectionContent, parameters, enumDefinitions);
    data.m_fileModifiedTime = Stats.m_LastModificationTime;
  }

  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = data.m_sShaderPath;
  desc.m_sPluginName = "ShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  for (auto& enumDef : enumDefinitions)
  {
    GetEnumType(enumDef);
  }

  for (auto& parameter : parameters)
  {
    const ezRTTI* pType = GetType(parameter);
    if (pType == nullptr)
    {
      continue;
    }

    ezBitflags<ezPropertyFlags> flags = ezPropertyFlags::Phantom;
    if (pType->IsDerivedFrom<ezEnumBase>())
      flags |= ezPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<ezBitflagsBase>())
      flags |= ezPropertyFlags::Bitflags;
    if (ezReflectionUtils::IsBasicType(pType))
      flags |= ezPropertyFlags::StandardType;

    ezReflectedPropertyDescriptor propDesc(ezPropertyCategory::Member, parameter.m_sName, pType->GetTypeName(), flags);

    AddAttributes(parameter, pType, propDesc.m_Attributes);

    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
  {
    // We do not want to listen to type changes that we triggered ourselves.
    data.m_pType = ezPhantomRttiManager::RegisterType(desc);
  }
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

void ezShaderTypeRegistry::PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType)
    {
      GetShaderType(e.m_pChangedType->GetTypeName());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

/// \brief Changes the base class of all shader types to ezShaderTypeBase (version 1) and
/// sets their own version to 2.
class ezShaderTypePatch_1_2 : public ezGraphPatch
{
public:
  ezShaderTypePatch_1_2()
    : ezGraphPatch(nullptr, 2, ezGraphPatch::PatchType::GraphPatch)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode*) const override
  {
    ezString sDescTypeName = ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetTypeName();

    auto& nodes = pGraph->GetAllNodes();
    bool bNeedAddBaseClass = false;
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      ezAbstractObjectNode* pNode = it.Value();
      if (pNode->GetType() == sDescTypeName)
      {
        auto* pTypeProperty = pNode->FindProperty("TypeName");
        if (ezStringUtils::EndsWith(pTypeProperty->m_Value.Get<ezString>(), ".ezShader"))
        {
          auto* pTypeVersionProperty = pNode->FindProperty("TypeVersion");
          auto* pParentTypeProperty = pNode->FindProperty("ParentTypeName");
          if (pTypeVersionProperty->m_Value == 1)
          {
            pParentTypeProperty->m_Value = "ezShaderTypeBase";
            pTypeVersionProperty->m_Value = (ezUInt32)2;
            bNeedAddBaseClass = true;
          }
        }
      }
    }

    if (bNeedAddBaseClass)
    {
      ezRttiConverterContext context;
      ezRttiConverterWriter rttiConverter(pGraph, &context, true, true);

      ezReflectedTypeDescriptor desc;
      desc.m_sTypeName = "ezShaderTypeBase";
      desc.m_sPluginName = "ShaderTypes";
      desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
      desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Abstract | ezTypeFlags::Class;
      desc.m_uiTypeVersion = 1;

      context.RegisterObject(ezUuid::MakeStableUuidFromString(desc.m_sTypeName.GetData()), ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
      rttiConverter.AddObjectToGraph(ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
    }
  }
};

ezShaderTypePatch_1_2 g_ezShaderTypePatch_1_2;

// TODO: Increase ezShaderTypeBase version to 2 and implement enum renames, see ezReflectedPropertyDescriptorPatch_1_2
class ezShaderBaseTypePatch_1_2 : public ezGraphPatch
{
public:
  ezShaderBaseTypePatch_1_2()
    : ezGraphPatch("ezShaderTypeBase", 2)
  {
  }

  static void FixEnumString(ezStringBuilder& ref_sValue, const char* szName)
  {
    if (ref_sValue.StartsWith(szName))
      ref_sValue.Shrink(ezStringUtils::GetCharacterCount(szName), 0);

    if (ref_sValue.StartsWith("::"))
      ref_sValue.Shrink(2, 0);

    if (ref_sValue.StartsWith(szName))
      ref_sValue.Shrink(ezStringUtils::GetCharacterCount(szName), 0);

    if (ref_sValue.StartsWith("_"))
      ref_sValue.Shrink(1, 0);

    ref_sValue.PrependFormat("{0}::{0}_", szName);
  }

  void FixEnum(ezAbstractObjectNode* pNode, const char* szEnum) const
  {
    if (ezAbstractObjectNode::Property* pProp = pNode->FindProperty(szEnum))
    {
      ezStringBuilder sValue = pProp->m_Value.Get<ezString>();
      FixEnumString(sValue, szEnum);
      pProp->m_Value = sValue.GetData();
    }
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    FixEnum(pNode, "SHADING_MODE");
    FixEnum(pNode, "BLEND_MODE");
    FixEnum(pNode, "RENDER_PASS");
  }
};

ezShaderBaseTypePatch_1_2 g_ezShaderBaseTypePatch_1_2;
