#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

namespace
{
  ezResult AddDefines(ezDynamicArray<ezString>& inout_defines, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
  {
    ezStringBuilder sDefine;

    const char* szName = pProp->GetPropertyName();
    if (pProp->GetSpecificType()->GetVariantType() == ezVariantType::Bool)
    {
      sDefine.Set(szName, " ", pObject->GetTypeAccessor().GetValue(szName).Get<bool>() ? "TRUE" : "FALSE");
      inout_defines.PushBack(sDefine);
      return EZ_SUCCESS;
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      ezInt64 iValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<ezInt64>();

      ezHybridArray<ezReflectionUtils::EnumKeyValuePair, 16> enumValues;
      ezReflectionUtils::GetEnumKeysAndValues(pProp->GetSpecificType(), enumValues, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& enumValue : enumValues)
      {
        sDefine.SetFormat("{} {}", enumValue.m_sKey, enumValue.m_iValue);
        inout_defines.PushBack(sDefine);

        if (enumValue.m_iValue == iValue)
        {
          sDefine.Set(szName, " ", enumValue.m_sKey);
          inout_defines.PushBack(sDefine);
        }
      }

      return EZ_SUCCESS;
    }

    EZ_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
    return EZ_FAILURE;
  }

  ezResult ParseMaterialConfig(ezStringView sRelativeFileName, const ezDocumentObject* pShaderPropertyObject, ezVariantDictionary& out_materialConfig)
  {
    ezFileReader file;
    if (file.Open(sRelativeFileName).Failed())
      return EZ_FAILURE;

    ezHybridArray<ezString, 16> defines;
    {
      ezHybridArray<const ezAbstractProperty*, 32> properties;
      pShaderPropertyObject->GetType()->GetAllProperties(properties);

      for (auto& pProp : properties)
      {
        const ezCategoryAttribute* pCategory = pProp->GetAttributeByType<ezCategoryAttribute>();
        if (pCategory == nullptr || ezStringUtils::IsEqual(pCategory->GetCategory(), "Permutation") == false)
          continue;

        EZ_SUCCEED_OR_RETURN(AddDefines(defines, pShaderPropertyObject, pProp));
      }
    }

    ezStringBuilder sOutput;
    EZ_SUCCEED_OR_RETURN(ezShaderParser::PreprocessSection(file, ezShaderHelper::ezShaderSections::MATERIALCONFIG, defines, sOutput));

    ezHybridArray<ezStringView, 32> allAssignments;
    sOutput.Split(false, allAssignments, "\n", ";", "\r");

    ezStringBuilder temp;
    ezHybridArray<ezStringView, 4> components;
    for (const ezStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        ezLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      out_materialConfig[components[0]] = components[1];
    }

    return EZ_SUCCESS;
  }
} // namespace

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMaterialAssetPreview, 1)
  EZ_ENUM_CONSTANT(ezMaterialAssetPreview::Ball),
  EZ_ENUM_CONSTANT(ezMaterialAssetPreview::Sphere),
  EZ_ENUM_CONSTANT(ezMaterialAssetPreview::Box),
  EZ_ENUM_CONSTANT(ezMaterialAssetPreview::Plane),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMaterialShaderMode, 1)
EZ_ENUM_CONSTANTS(ezMaterialShaderMode::BaseMaterial, ezMaterialShaderMode::File, ezMaterialShaderMode::Custom)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetProperties, 4, ezRTTIDefaultAllocator<ezMaterialAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("ShaderMode", ezMaterialShaderMode, GetShaderMode, SetShaderMode),
    EZ_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", ezDependencyFlags::Transform | ezDependencyFlags::Thumbnail | ezDependencyFlags::Package)),
    EZ_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("AssetFilterTags", m_sAssetFilterTags),
    EZ_ACCESSOR_PROPERTY("Shader", GetShader, SetShader)->AddAttributes(new ezFileBrowserAttribute("Select Shader", "*.ezShader", "CustomAction_CreateShaderFromTemplate")),
    // This property holds the phantom shader properties type so it is only used in the object graph but not actually in the instance of this object.
    EZ_ACCESSOR_PROPERTY("ShaderProperties", GetShaderProperties, SetShaderProperties)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocument, 7, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezUuid ezMaterialAssetDocument::s_LitBaseMaterial;
ezUuid ezMaterialAssetDocument::s_LitAlphaTextBaseMaterial;
ezUuid ezMaterialAssetDocument::s_NeutralNormalMap;

void ezMaterialAssetProperties::SetBaseMaterial(const char* szBaseMaterial)
{
  if (m_sBaseMaterial == szBaseMaterial)
    return;

  m_sBaseMaterial = szBaseMaterial;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  if (m_pDocument->GetCommandHistory()->IsInUndoRedo())
    return;
  m_pDocument->SetBaseMaterial(m_sBaseMaterial);
}

const char* ezMaterialAssetProperties::GetBaseMaterial() const
{
  return m_sBaseMaterial;
}

void ezMaterialAssetProperties::SetShader(const char* szShader)
{
  if (m_sShader != szShader)
  {
    m_sShader = szShader;
    UpdateShader();
  }
}

const char* ezMaterialAssetProperties::GetShader() const
{
  return m_sShader;
}

void ezMaterialAssetProperties::SetShaderProperties(ezReflectedClass* pProperties)
{
  // This property represents the phantom shader type, so it is never actually used.
}

ezReflectedClass* ezMaterialAssetProperties::GetShaderProperties() const
{
  // This property represents the phantom shader type, so it is never actually used.
  return nullptr;
}

void ezMaterialAssetProperties::SetShaderMode(ezEnum<ezMaterialShaderMode> mode)
{
  if (m_ShaderMode == mode)
    return;

  m_ShaderMode = mode;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  ezCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  ezObjectAccessorBase* pAccessor = m_pDocument->GetObjectAccessor();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  ezStringBuilder tmp;

  switch (m_ShaderMode)
  {
    case ezMaterialShaderMode::BaseMaterial:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case ezMaterialShaderMode::File:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case ezMaterialShaderMode::Custom:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", ezConversionUtils::ToString(m_pDocument->GetGuid(), tmp).GetData()).AssertSuccess();
    }
    break;
  }
}

void ezMaterialAssetProperties::SetDocument(ezMaterialAssetDocument* pDocument)
{
  m_pDocument = pDocument;
  if (!m_sBaseMaterial.IsEmpty())
  {
    m_pDocument->SetBaseMaterial(m_sBaseMaterial);
  }
  UpdateShader(true);
}


void ezMaterialAssetProperties::UpdateShader(bool bForce)
{
  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;

  ezCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  EZ_ASSERT_DEBUG(pHistory->IsInTransaction(), "Missing undo scope on stack.");

  ezDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();

  // TODO: If m_sShader is empty, we need to get the shader of our base material and use that one instead
  // for the code below. The type name is the clean path to the shader at the moment.
  ezStringBuilder sShaderPath = ResolveRelativeShaderPath();
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsEmpty())
  {
    // No shader, delete any existing properties object.
    if (pPropObject)
    {
      DeleteProperties();
    }
  }
  else
  {
    if (pPropObject)
    {
      // We already have a shader properties object, test whether
      // it has a different type than the newly set shader. The type name
      // is the clean path to the shader at the moment.
      const ezRTTI* pType = pPropObject->GetTypeAccessor().GetType();
      if (sShaderPath != pType->GetTypeName() || bForce) // TODO: Is force even necessary anymore?
      {
        // Shader has changed, delete old and create new one.
        DeleteProperties();
        CreateProperties(sShaderPath);
      }
      else
      {
        // Same shader but it could have changed so try to update it anyway.
        ezShaderTypeRegistry::GetSingleton()->GetShaderType(sShaderPath);
      }
    }

    if (!pPropObject)
    {
      // No shader properties exist yet, so create a new one.
      CreateProperties(sShaderPath);
    }
  }
}

void ezMaterialAssetProperties::DeleteProperties()
{
  SaveOldValues();
  ezCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  ezDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  ezRemoveObjectCommand cmd;
  cmd.m_Object = pPropObject->GetGuid();
  auto res = pHistory->AddCommand(cmd);
  EZ_ASSERT_DEV(res.m_Result.Succeeded(), "Removal of old properties should never fail.");
}

void ezMaterialAssetProperties::CreateProperties(const char* szShaderPath)
{
  ezCommandHistory* pHistory = m_pDocument->GetCommandHistory();

  const ezRTTI* pType = ezShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  if (!pType && m_ShaderMode == ezMaterialShaderMode::Custom)
  {
    // Force generate if custom shader is missing
    ezAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(0, m_pDocument->GetAssetTypeVersion());
    m_pDocument->RecreateVisualShaderFile(AssetHeader).LogFailure();
    pType = ezShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  }

  if (pType)
  {
    ezAddObjectCommand cmd;
    cmd.m_pType = pType;
    cmd.m_sParentProperty = "ShaderProperties";
    cmd.m_Parent = m_pDocument->GetPropertyObject()->GetGuid();
    cmd.m_NewObjectGuid = cmd.m_Parent;
    cmd.m_NewObjectGuid.CombineWithSeed(ezUuid::MakeStableUuidFromString("ShaderProperties"));

    auto res = pHistory->AddCommand(cmd);
    EZ_ASSERT_DEV(res.m_Result.Succeeded(), "Addition of new properties should never fail.");
    LoadOldValues();
  }
}

void ezMaterialAssetProperties::SaveOldValues()
{
  ezDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  if (pPropObject)
  {
    const ezIReflectedTypeAccessor& accessor = pPropObject->GetTypeAccessor();
    const ezRTTI* pType = accessor.GetType();
    ezHybridArray<const ezAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        m_CachedProperties[pProp->GetPropertyName()] = accessor.GetValue(pProp->GetPropertyName());
      }
    }
  }
}

void ezMaterialAssetProperties::LoadOldValues()
{
  ezDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  ezCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  if (pPropObject)
  {
    const ezIReflectedTypeAccessor& accessor = pPropObject->GetTypeAccessor();
    const ezRTTI* pType = accessor.GetType();
    ezHybridArray<const ezAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        ezString sPropName = pProp->GetPropertyName();
        auto it = m_CachedProperties.Find(sPropName);
        if (it.IsValid())
        {
          if (it.Value() != accessor.GetValue(sPropName.GetData()))
          {
            ezSetObjectPropertyCommand cmd;
            cmd.m_Object = pPropObject->GetGuid();
            cmd.m_sProperty = sPropName;
            cmd.m_NewValue = it.Value();

            // Do not check for success, if a cached value failed to apply, simply ignore it.
            pHistory->AddCommand(cmd).AssertSuccess();
          }
        }
      }
    }
  }
}

ezString ezMaterialAssetProperties::GetAutoGenShaderPathAbs() const
{
  ezAssetDocumentManager* pManager = ezDynamicCast<ezAssetDocumentManager*>(m_pDocument->GetDocumentManager());
  ezString sAbsOutputPath = pManager->GetAbsoluteOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), m_pDocument->GetDocumentPath(), ezMaterialAssetDocumentManager::s_szShaderOutputTag);
  return sAbsOutputPath;
}

void ezMaterialAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezMaterialAssetProperties>())
  {
    ezInt64 shaderMode = e.m_pObject->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    if (shaderMode == ezMaterialShaderMode::File)
      props["Shader"].m_Visibility = ezPropertyUiState::Default;
    else
      props["Shader"].m_Visibility = ezPropertyUiState::Invisible;

    if (shaderMode == ezMaterialShaderMode::BaseMaterial)
      props["BaseMaterial"].m_Visibility = ezPropertyUiState::Default;
    else
      props["BaseMaterial"].m_Visibility = ezPropertyUiState::Invisible;
  }
}

ezString ezMaterialAssetProperties::ResolveRelativeShaderPath() const
{
  bool isGuid = ezConversionUtils::IsStringUuid(m_sShader);

  if (isGuid)
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(m_sShader);
    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guid);
    if (pAsset)
    {
      EZ_ASSERT_DEV(pAsset->m_pAssetInfo->GetManager() == m_pDocument->GetDocumentManager(), "Referenced shader via guid by this material is not of type material asset (ezMaterialShaderMode::Custom).");

      ezStringBuilder sProjectDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(pAsset->m_pAssetInfo->m_Path);
      ezStringBuilder sResult = pAsset->m_pAssetInfo->GetManager()->GetRelativeOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), sProjectDir, pAsset->m_pAssetInfo->m_Path, ezMaterialAssetDocumentManager::s_szShaderOutputTag);

      sResult.Prepend("AssetCache/");
      return sResult;
    }
    else
    {
      ezLog::Error("Could not resolve guid '{0}' for the material shader.", m_sShader);
      return "";
    }
  }
  else
  {
    return m_sShader;
  }

  return m_sShader;
}

//////////////////////////////////////////////////////////////////////////

ezMaterialAssetDocument::ezMaterialAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezMaterialAssetProperties>(EZ_DEFAULT_NEW(ezMaterialObjectManager), sDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
  ezQtEditorApp::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezMaterialAssetDocument::EditorEventHandler, this));
}

ezMaterialAssetDocument::~ezMaterialAssetDocument()
{
  ezQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezMaterialAssetDocument::EditorEventHandler, this));
}

void ezMaterialAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  {
    ezCommandHistory* pHistory = GetCommandHistory();
    pHistory->StartTransaction("Update Material Shader");
    GetProperties()->SetDocument(this);
    pHistory->FinishTransaction();
  }

  bool bSetModified = false;

  // The above command may patch the doc with the newest shader properties so we need to clear the undo history here.
  GetCommandHistory()->ClearUndoHistory();
  SetModified(bSetModified);
}

ezDocumentObject* ezMaterialAssetDocument::GetShaderPropertyObject()
{
  ezDocumentObject* pObject = GetObjectManager()->GetRootObject()->GetChildren()[0];
  ezIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  ezUuid propObjectGuid = accessor.GetValue("ShaderProperties").ConvertTo<ezUuid>();
  ezDocumentObject* pPropObject = nullptr;
  if (propObjectGuid.IsValid())
  {
    pPropObject = GetObjectManager()->GetObject(propObjectGuid);
  }
  return pPropObject;
}

const ezDocumentObject* ezMaterialAssetDocument::GetShaderPropertyObject() const
{
  return const_cast<ezMaterialAssetDocument*>(this)->GetShaderPropertyObject();
}

void ezMaterialAssetDocument::SetBaseMaterial(const char* szBaseMaterial)
{
  ezDocumentObject* pObject = GetPropertyObject();
  auto pAssetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(szBaseMaterial);
  if (pAssetInfo == nullptr)
  {
    ezHybridArray<const ezDocumentObject*, 2> sel;
    sel.PushBack(pObject);
    UnlinkPrefabs(sel);
  }
  else
  {
    const ezStringBuilder& sNewBase = ezPrefabCache::GetSingleton()->GetCachedPrefabDocument(pAssetInfo->m_Data.m_Guid);
    const ezAbstractObjectGraph* pBaseGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(pAssetInfo->m_Data.m_Guid);

    ezUuid seed = GetSeedFromBaseMaterial(pBaseGraph);
    if (sNewBase.IsEmpty() || !pBaseGraph || !seed.IsValid())
    {
      ezLog::Error("The selected base material '{0}' is not a valid material file!", szBaseMaterial);
      return;
    }

    {
      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());

      if (pMeta->m_CreateFromPrefab != pAssetInfo->m_Data.m_Guid)
      {
        pMeta->m_sBasePrefab = sNewBase;
        pMeta->m_CreateFromPrefab = pAssetInfo->m_Data.m_Guid;
        pMeta->m_PrefabSeedGuid = seed;
      }
      m_DocumentObjectMetaData->EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
    }
    UpdatePrefabs();
  }
}

ezUuid ezMaterialAssetDocument::GetSeedFromBaseMaterial(const ezAbstractObjectGraph* pBaseGraph)
{
  if (!pBaseGraph)
    return ezUuid();

  ezUuid instanceGuid = GetPropertyObject()->GetGuid();
  ezUuid baseGuid = ezMaterialAssetDocument::GetMaterialNodeGuid(*pBaseGraph);
  if (baseGuid.IsValid())
  {
    // Create seed that converts base guid into instance guid
    instanceGuid.RevertCombinationWithSeed(baseGuid);
    return instanceGuid;
  }

  return ezUuid();
}

ezUuid ezMaterialAssetDocument::GetMaterialNodeGuid(const ezAbstractObjectGraph& graph)
{
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == ezGetStaticRTTI<ezMaterialAssetProperties>()->GetTypeName())
    {
      return it.Value()->GetGuid();
    }
  }
  return ezUuid();
}

void ezMaterialAssetDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, ezStringView sBasePrefab)
{
  // Base
  ezAbstractObjectGraph baseGraph;
  ezPrefabUtils::LoadGraph(baseGraph, sBasePrefab);
  baseGraph.PruneGraph(GetMaterialNodeGuid(baseGraph));

  // NewBase
  const ezStringBuilder& sLeft = ezPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);
  const ezAbstractObjectGraph* pLeftGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(PrefabAsset);
  ezAbstractObjectGraph leftGraph;
  if (pLeftGraph)
  {
    pLeftGraph->Clone(leftGraph);
  }
  else
  {
    ezStringBuilder sGuid;
    ezConversionUtils::ToString(PrefabAsset, sGuid);
    ezLog::Error("Can't update prefab, new base graph does not exist: {0}", sGuid);
    return;
  }
  leftGraph.PruneGraph(GetMaterialNodeGuid(leftGraph));

  // Instance
  ezAbstractObjectGraph rightGraph;
  {
    ezDocumentObjectConverterWriter writer(&rightGraph, pObject->GetDocumentObjectManager());
    writer.AddObjectToGraph(pObject);
    rightGraph.ReMapNodeGuids(PrefabSeed, true);
  }

  // Merge diffs relative to base
  ezDeque<ezAbstractGraphDiffOperation> mergedDiff;
  ezPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);

  // Skip 'ShaderMode' as it should not be inherited, and 'ShaderProperties' is being set by the 'Shader' property
  ezDeque<ezAbstractGraphDiffOperation> cleanedDiff;
  for (const ezAbstractGraphDiffOperation& op : mergedDiff)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      if (op.m_sProperty == "ShaderMode" || op.m_sProperty == "ShaderProperties")
        continue;

      cleanedDiff.PushBack(op);
    }
  }

  // Apply diff to base, making it the new instance
  baseGraph.ApplyDiff(cleanedDiff);

  // Do not allow 'Shader' to be overridden, always use the prefab template version.
  if (ezAbstractObjectNode* pNode = leftGraph.GetNode(GetMaterialNodeGuid(leftGraph)))
  {
    if (auto pProp = pNode->FindProperty("Shader"))
    {
      if (ezAbstractObjectNode* pNodeBase = baseGraph.GetNode(GetMaterialNodeGuid(baseGraph)))
      {
        pNodeBase->ChangeProperty("Shader", pProp->m_Value);
      }
    }
  }

  // Create a new diff that changes our current instance to the new instance
  ezDeque<ezAbstractGraphDiffOperation> newInstanceToCurrentInstance;
  baseGraph.CreateDiffWithBaseGraph(rightGraph, newInstanceToCurrentInstance);
  if (false)
  {
    ezFileWriter file;
    file.Open("C:\\temp\\Material - diff.txt").IgnoreResult();

    ezStringBuilder sDiff;
    sDiff.Append("######## New Instance To Instance #######\n");
    ezPrefabUtils::WriteDiff(newInstanceToCurrentInstance, sDiff);
    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
  // Apply diff to current instance
  // Shader needs to be set first
  for (ezUInt32 i = 0; i < newInstanceToCurrentInstance.GetCount(); ++i)
  {
    if (newInstanceToCurrentInstance[i].m_sProperty == "Shader")
    {
      ezAbstractGraphDiffOperation op = newInstanceToCurrentInstance[i];
      newInstanceToCurrentInstance.RemoveAtAndCopy(i);
      newInstanceToCurrentInstance.InsertAt(0, op);
      break;
    }
  }
  for (const ezAbstractGraphDiffOperation& op : newInstanceToCurrentInstance)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      // Never change this material's mode, as it should not be inherited from prefab base
      if (op.m_sProperty == "ShaderMode")
        continue;

      // these properties may not exist and we do not want to change them either
      if (op.m_sProperty == "MetaBasePrefab" || op.m_sProperty == "MetaPrefabSeed" || op.m_sProperty == "MetaFromPrefab")
        continue;

      ezSetObjectPropertyCommand cmd;
      cmd.m_Object = op.m_Node;
      cmd.m_Object.CombineWithSeed(PrefabSeed);
      cmd.m_NewValue = op.m_Value;
      cmd.m_sProperty = op.m_sProperty;

      auto pObj = GetObjectAccessor()->GetObject(cmd.m_Object);
      if (!pObj)
        continue;

      auto pProp = pObj->GetType()->FindPropertyByName(op.m_sProperty);
      if (!pProp)
        continue;

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        continue;

      GetCommandHistory()->AddCommand(cmd).AssertSuccess();
    }
  }

  // Update prefab meta data
  {
    auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMeta->m_CreateFromPrefab = PrefabAsset; // Should not change
    pMeta->m_PrefabSeedGuid = PrefabSeed;    // Should not change
    pMeta->m_sBasePrefab = sLeft;

    m_DocumentObjectMetaData->EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }
}

class ezVisualShaderErrorLog : public ezLogInterface
{
public:
  ezStringBuilder m_sResult;
  ezResult m_Status;

  ezVisualShaderErrorLog()
    : m_Status(EZ_SUCCESS)
  {
  }

  virtual void HandleLogMessage(const ezLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case ezLogMsgType::ErrorMsg:
        m_Status = EZ_FAILURE;
        m_sResult.Append("Error: ", le.m_sText, "\n");
        break;

      case ezLogMsgType::SeriousWarningMsg:
      case ezLogMsgType::WarningMsg:
        m_sResult.Append("Warning: ", le.m_sText, "\n");
        break;

      default:
        return;
    }
  }
};

ezTransformStatus ezMaterialAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  if (sOutputTag.IsEqual(ezMaterialAssetDocumentManager::s_szShaderOutputTag))
  {
    ezStatus ret = RecreateVisualShaderFile(AssetHeader);

    if (transformFlags.IsSet(ezTransformFlags::ForceTransform))
    {
      ezMaterialVisualShaderEvent e;

      if (GetProperties()->m_ShaderMode == ezMaterialShaderMode::Custom)
      {
        e.m_Type = ezMaterialVisualShaderEvent::TransformFailed;
        e.m_sTransformError = ret.m_sMessage;

        if (ret.Succeeded())
        {
          e.m_Type = ezMaterialVisualShaderEvent::TransformSucceeded;
          ezStringBuilder sAutoGenShader = GetProperties()->GetAutoGenShaderPathAbs();

          QStringList arguments;
          ezStringBuilder temp;

          arguments << "-project";
          arguments << QString::fromUtf8(ezToolsProject::GetSingleton()->GetProjectDirectory().GetData());

          arguments << "-shader";
          arguments << QString::fromUtf8(sAutoGenShader.GetData());

          arguments << "-platform";
          arguments << "DX11_SM50"; /// \todo Rendering platform is currently hardcoded

          // determine the permutation variables that should get fixed values
          {
            // m_sCheckPermutations are just all fixed perm vars from every node in the VS
            ezStringBuilder temp = m_sCheckPermutations;
            ezDeque<ezStringView> perms;
            temp.Split(false, perms, "\n");

            // remove duplicates
            ezSet<ezString> uniquePerms;
            for (const ezStringView& perm : perms)
            {
              uniquePerms.Insert(perm);
            }

            // pass permutation variable definitions to the compiler: "SOME_VAR=SOME_VAL"
            arguments << "-perm";
            for (auto it = uniquePerms.GetIterator(); it.IsValid(); ++it)
            {
              arguments << it.Key().GetData();
            }
          }

          ezVisualShaderErrorLog log;

          ret = ezQtEditorApp::GetSingleton()->ExecuteTool("ezShaderCompiler", arguments, 60, &log);
          if (ret.Failed())
          {
            e.m_Type = ezMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = ret.m_sMessage;
          }
          else
          {
            e.m_Type = log.m_Status.Succeeded() ? ezMaterialVisualShaderEvent::TransformSucceeded : ezMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = log.m_sResult;
            ezLog::Info("Compiled Visual Shader.");
          }
        }
      }
      else
      {
        e.m_Type = ezMaterialVisualShaderEvent::VisualShaderNotUsed;
      }

      if (e.m_Type == ezMaterialVisualShaderEvent::TransformFailed)
      {
        TagVisualShaderFileInvalid(pAssetProfile, e.m_sTransformError);
      }

      m_VisualShaderEvents.Broadcast(e);
    }

    return ret;
  }
  else
  {
    return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  }
}

ezTransformStatus ezMaterialAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_ASSERT_DEV(sOutputTag.IsEmpty(), "Additional output '{0}' not implemented!", sOutputTag);

  return WriteMaterialAsset(stream, pAssetProfile, true);
}

ezTransformStatus ezMaterialAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  return ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
}

void ezMaterialAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezMaterialAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezMaterialAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void ezMaterialAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (!GetProperties()->m_sAssetFilterTags.IsEmpty())
  {
    const ezStringBuilder tags(";", GetProperties()->m_sAssetFilterTags, ";");

    pInfo->m_sAssetsDocumentTags = tags;
  }

  if (GetProperties()->m_ShaderMode != ezMaterialShaderMode::BaseMaterial)
  {
    // remove base material dependency, if it isn't used
    pInfo->m_TransformDependencies.Remove(GetProperties()->GetBaseMaterial());
    pInfo->m_ThumbnailDependencies.Remove(GetProperties()->GetBaseMaterial());
  }

  if (GetProperties()->m_ShaderMode != ezMaterialShaderMode::File)
  {
    const bool bInUseByBaseMaterial = GetProperties()->m_ShaderMode == ezMaterialShaderMode::BaseMaterial && ezStringUtils::IsEqual(GetProperties()->GetShader(), GetProperties()->GetBaseMaterial());

    // remove shader file dependency, if it isn't used and differs from the base material
    if (!bInUseByBaseMaterial)
    {
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetShader());
      pInfo->m_ThumbnailDependencies.Remove(GetProperties()->GetShader());
    }
  }

  if (GetProperties()->m_ShaderMode == ezMaterialShaderMode::Custom)
  {
    // We write our own guid into the shader field so BaseMaterial materials can find the shader file.
    // This would cause us to have a dependency to ourselves so we need to remove it.
    ezStringBuilder tmp;
    pInfo->m_TransformDependencies.Remove(ezConversionUtils::ToString(GetGuid(), tmp));
    pInfo->m_ThumbnailDependencies.Remove(ezConversionUtils::ToString(GetGuid(), tmp));

    ezVisualShaderCodeGenerator codeGen;

    ezSet<ezString> cfgFiles;
    codeGen.DetermineConfigFileDependencies(static_cast<const ezDocumentNodeManager*>(GetObjectManager()), cfgFiles);

    for (const auto& sCfgFile : cfgFiles)
    {
      pInfo->m_TransformDependencies.Insert(sCfgFile);
    }

    pInfo->m_Outputs.Insert(ezMaterialAssetDocumentManager::s_szShaderOutputTag);

    /// \todo The Visual Shader node configuration files would need to be a dependency of the auto-generated shader.
  }
}

ezStatus ezMaterialAssetDocument::WriteMaterialAsset(ezStreamWriter& inout_stream0, const ezPlatformProfile* pAssetProfile, bool bEmbedLowResData) const
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  ezStringBuilder sValue;

  // now generate the .ezBinMaterial file
  {
    const ezUInt8 uiVersion = 7;

    inout_stream0 << uiVersion;

    ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    uiCompressionMode = 1;
    ezCompressedStreamWriterZstd stream(&inout_stream0, 0, ezCompressedStreamWriterZstd::Compression::Average);
#else
    ezStreamWriter& stream = stream0;
#endif

    inout_stream0 << uiCompressionMode;

    stream << pProp->m_sBaseMaterial;
    stream << pProp->m_sSurface;

    ezString sRelativeShaderPath = pProp->ResolveRelativeShaderPath();
    stream << sRelativeShaderPath;

    ezHybridArray<const ezAbstractProperty*, 16> Textures2D;
    ezHybridArray<const ezAbstractProperty*, 16> TexturesCube;
    ezHybridArray<const ezAbstractProperty*, 16> Permutations;
    ezHybridArray<const ezAbstractProperty*, 16> Constants;

    const ezDocumentObject* pObject = GetShaderPropertyObject();
    if (pObject != nullptr)
    {
      bool hasBaseMaterial = ezPrefabUtils::GetPrefabRoot(pObject, *m_DocumentObjectMetaData).IsValid();
      auto pType = pObject->GetTypeAccessor().GetType();
      ezHybridArray<const ezAbstractProperty*, 32> properties;
      pType->GetAllProperties(properties);

      ezHybridArray<ezPropertySelection, 1> selection;
      selection.PushBack({pObject, ezVariant()});
      ezDefaultObjectState defaultState(GetObjectAccessor(), selection.GetArrayPtr());

      for (auto pProp : properties)
      {
        if (hasBaseMaterial && defaultState.IsDefaultValue(pProp))
          continue;

        const ezCategoryAttribute* pCategory = pProp->GetAttributeByType<ezCategoryAttribute>();

        EZ_ASSERT_DEBUG(pCategory, "Category cannot be null for a shader property");
        if (pCategory == nullptr)
          continue;

        if (ezStringUtils::IsEqual(pCategory->GetCategory(), "Texture 2D"))
        {
          Textures2D.PushBack(pProp);
        }
        else if (ezStringUtils::IsEqual(pCategory->GetCategory(), "Texture Cube"))
        {
          TexturesCube.PushBack(pProp);
        }
        else if (ezStringUtils::IsEqual(pCategory->GetCategory(), "Permutation"))
        {
          Permutations.PushBack(pProp);
        }
        else if (ezStringUtils::IsEqual(pCategory->GetCategory(), "Constant"))
        {
          Constants.PushBack(pProp);
        }
        else
        {
          EZ_REPORT_FAILURE("Invalid shader property type '{0}'", pCategory->GetCategory());
        }
      }
    }

    // write out the permutation variables
    {
      const ezUInt16 uiPermVars = Permutations.GetCount();
      stream << uiPermVars;

      for (auto pProp : Permutations)
      {
        EZ_ASSERT_DEBUG(pObject != nullptr, "Need object to write out permutation");
        const char* szName = pProp->GetPropertyName();
        if (pProp->GetSpecificType()->GetVariantType() == ezVariantType::Bool)
        {
          sValue = pObject->GetTypeAccessor().GetValue(szName).Get<bool>() ? "TRUE" : "FALSE";
        }
        else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        {
          ezReflectionUtils::EnumerationToString(pProp->GetSpecificType(), pObject->GetTypeAccessor().GetValue(szName).ConvertTo<ezInt64>(), sValue, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
        }
        else
        {
          EZ_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
        }

        stream << szName;
        stream << sValue;
      }
    }

    // write out the 2D textures
    {
      const ezUInt16 uiTextures = Textures2D.GetCount();
      stream << uiTextures;

      for (auto pProp : Textures2D)
      {
        EZ_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture");
        const char* szName = pProp->GetPropertyName();
        sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<ezString>();

        stream << szName;
        stream << sValue;
      }
    }

    // write out the Cube textures
    {
      const ezUInt16 uiTextures = TexturesCube.GetCount();
      stream << uiTextures;

      for (auto pProp : TexturesCube)
      {
        EZ_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture cube");
        const char* szName = pProp->GetPropertyName();
        sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<ezString>();

        stream << szName;
        stream << sValue;
      }
    }

    // write out the constants
    {
      const ezUInt16 uiConstants = Constants.GetCount();
      stream << uiConstants;

      for (auto pProp : Constants)
      {
        EZ_ASSERT_DEBUG(pObject != nullptr, "Need object to write out constant");
        const char* szName = pProp->GetPropertyName();
        ezVariant value = pObject->GetTypeAccessor().GetValue(szName);

        stream << szName;
        stream << value;
      }
    }

    // render data category
    {
      ezVariantDictionary materialConfig;
      if (pObject != nullptr)
      {
        EZ_SUCCEED_OR_RETURN(ParseMaterialConfig(sRelativeShaderPath, pObject, materialConfig));
      }

      ezVariant renderDataCategory;
      materialConfig.TryGetValue("RenderDataCategory", renderDataCategory);

      stream << renderDataCategory.ConvertTo<ezString>();
    }

    // find and embed low res texture data
    {
      if (bEmbedLowResData)
      {
        ezStringBuilder sFilename, sResourceName;
        ezDynamicArray<ezUInt32> content;

        // embed 2D texture data
        for (auto prop : Textures2D)
        {
          EZ_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture2d");
          const char* szName = prop->GetPropertyName();
          sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<ezString>();

          if (sValue.IsEmpty())
            continue;

          sResourceName = sValue;

          auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(sValue);
          if (!asset.isValid())
            continue;

          sValue = asset->m_pAssetInfo->GetManager()->GetAbsoluteOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, asset->m_pAssetInfo->m_Path, "", pAssetProfile);

          sFilename = sValue.GetFileName();
          sFilename.Append("-lowres");

          sValue.ChangeFileName(sFilename);

          ezFileReader file;
          if (file.Open(sValue).Failed())
            continue;

          content.SetCountUninitialized(file.GetFileSize());

          file.ReadBytes(content.GetData(), content.GetCount());

          stream << sResourceName;
          stream << content.GetCount();
          EZ_SUCCEED_OR_RETURN(stream.WriteBytes(content.GetData(), content.GetCount()));
        }
      }

      // marker: end of embedded data
      stream << "";
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    EZ_SUCCEED_OR_RETURN(stream.FinishCompressedStream());

    ezLog::Dev("Compressed material data from {0} KB to {1} KB ({2}%%)", ezArgF((float)stream.GetUncompressedSize() / 1024.0f, 1), ezArgF((float)stream.GetCompressedSize() / 1024.0f, 1), ezArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
  }

  return ezStatus(EZ_SUCCESS);
}

void ezMaterialAssetDocument::TagVisualShaderFileInvalid(const ezPlatformProfile* pAssetProfile, const char* szError)
{
  if (GetProperties()->m_ShaderMode != ezMaterialShaderMode::Custom)
    return;

  ezAssetDocumentManager* pManager = ezDynamicCast<ezAssetDocumentManager*>(GetDocumentManager());
  ezString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), ezMaterialAssetDocumentManager::s_szShaderOutputTag);

  ezStringBuilder all;

  // read shader source
  {
    ezFileReader file;
    if (file.Open(sAutoGenShader).Failed())
      return;

    all.ReadAll(file);
  }

  all.PrependFormat("/*\n{0}\n*/\n", szError);

  // write adjusted shader source
  {
    ezFileWriter fileOut;
    if (fileOut.Open(sAutoGenShader).Failed())
      return;

    fileOut.WriteBytes(all.GetData(), all.GetElementCount()).IgnoreResult();
  }
}

ezStatus ezMaterialAssetDocument::RecreateVisualShaderFile(const ezAssetFileHeader& assetHeader)
{
  if (GetProperties()->m_ShaderMode != ezMaterialShaderMode::Custom)
  {
    return ezStatus(EZ_SUCCESS);
  }

  ezAssetDocumentManager* pManager = ezDynamicCast<ezAssetDocumentManager*>(GetDocumentManager());
  ezString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), ezMaterialAssetDocumentManager::s_szShaderOutputTag);

  ezVisualShaderCodeGenerator codeGen;

  EZ_SUCCEED_OR_RETURN(codeGen.GenerateVisualShader(static_cast<const ezDocumentNodeManager*>(GetObjectManager()), m_sCheckPermutations));

  ezFileWriter file;
  if (file.Open(sAutoGenShader).Succeeded())
  {
    ezStringBuilder shader = codeGen.GetFinalShaderCode();
    shader.PrependFormat("//{0}|{1}\n", assetHeader.GetFileHash(), assetHeader.GetFileVersion());

    EZ_SUCCEED_OR_RETURN(file.WriteBytes(shader.GetData(), shader.GetElementCount()));
    file.Close();

    InvalidateCachedShader();

    return ezStatus(EZ_SUCCESS);
  }
  else
    return ezStatus(ezFmt("Failed to write auto-generated shader to '{0}'", sAutoGenShader));
}

void ezMaterialAssetDocument::InvalidateCachedShader()
{
  ezAssetDocumentManager* pManager = ezDynamicCast<ezAssetDocumentManager*>(GetDocumentManager());
  ezString sShader;

  if (GetProperties()->m_ShaderMode == ezMaterialShaderMode::Custom)
  {
    sShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), ezMaterialAssetDocumentManager::s_szShaderOutputTag);
  }
  else
  {
    sShader = GetProperties()->GetShader();
  }

  // This should update the shader parameter section in all affected materials
  ezShaderTypeRegistry::GetSingleton()->GetShaderType(sShader);
}

void ezMaterialAssetDocument::EditorEventHandler(const ezEditorAppEvent& e)
{
  if (e.m_Type == ezEditorAppEvent::Type::ReloadResources)
  {
    InvalidateCachedShader();
  }
}

static void MarkReachableNodes(ezMap<const ezDocumentObject*, bool>& ref_allNodes, const ezDocumentObject* pRoot, ezDocumentNodeManager* pNodeManager)
{
  if (ref_allNodes[pRoot])
    return;

  ref_allNodes[pRoot] = true;

  auto allInputs = pNodeManager->GetInputPins(pRoot);

  // we start at the final output, so use the inputs on a node and then walk backwards
  for (auto& pTargetPin : allInputs)
  {
    auto connections = pNodeManager->GetConnections(*pTargetPin);

    // all incoming connections at the input pin, there should only be one though
    for (const ezConnection* const pConnection : connections)
    {
      // output pin on other node connecting to this node
      const ezPin& sourcePin = pConnection->GetSourcePin();

      // recurse from here
      MarkReachableNodes(ref_allNodes, sourcePin.GetParent(), pNodeManager);
    }
  }
}

void ezMaterialAssetDocument::RemoveDisconnectedNodes()
{
  ezDocumentNodeManager* pNodeManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());

  const ezDocumentObject* pRoot = pNodeManager->GetRootObject();
  const ezRTTI* pNodeBaseRtti = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  const ezHybridArray<ezDocumentObject*, 8>& children = pRoot->GetChildren();
  ezMap<const ezDocumentObject*, bool> AllNodes;

  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    if (children[i]->GetType()->IsDerivedFrom(pNodeBaseRtti))
    {
      AllNodes[children[i]] = false;
    }
  }

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    // skip nodes that have already been marked
    if (it.Value())
      continue;

    auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(it.Key()->GetType());

    if (pDesc->m_NodeType == ezVisualShaderNodeType::Main)
    {
      MarkReachableNodes(AllNodes, it.Key(), pNodeManager);
    }
  }

  // now purge all nodes that haven't been reached
  {
    auto pHistory = GetCommandHistory();
    pHistory->StartTransaction("Purge unreachable nodes");

    for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
    {
      // skip nodes that have been marked
      if (it.Value())
        continue;

      ezRemoveNodeCommand rem;
      rem.m_Object = it.Key()->GetGuid();

      pHistory->AddCommand(rem).AssertSuccess();
    }

    pHistory->FinishTransaction();
  }
}

ezUuid ezMaterialAssetDocument::GetLitBaseMaterial()
{
  if (!s_LitBaseMaterial.IsValid())
  {
    static const char* szLitMaterialAssetPath = ezMaterialResource::GetDefaultMaterialFileName(ezMaterialResource::DefaultMaterialType::Lit);
    auto assetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(szLitMaterialAssetPath);
    if (assetInfo)
      s_LitBaseMaterial = assetInfo->m_Data.m_Guid;
    else
      ezLog::Error("Can't find default lit material {0}", szLitMaterialAssetPath);
  }
  return s_LitBaseMaterial;
}

ezUuid ezMaterialAssetDocument::GetLitAlphaTestBaseMaterial()
{
  if (!s_LitAlphaTextBaseMaterial.IsValid())
  {
    static const char* szLitAlphaTestMaterialAssetPath = ezMaterialResource::GetDefaultMaterialFileName(ezMaterialResource::DefaultMaterialType::LitAlphaTest);
    auto assetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(szLitAlphaTestMaterialAssetPath);
    if (assetInfo)
      s_LitAlphaTextBaseMaterial = assetInfo->m_Data.m_Guid;
    else
      ezLog::Error("Can't find default lit alpha test material {0}", szLitAlphaTestMaterialAssetPath);
  }
  return s_LitAlphaTextBaseMaterial;
}

ezUuid ezMaterialAssetDocument::GetNeutralNormalMap()
{
  if (!s_NeutralNormalMap.IsValid())
  {
    static const char* szNeutralNormalMapAssetPath = "Base/Textures/NeutralNormal.ezTextureAsset";
    auto assetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(szNeutralNormalMapAssetPath);
    if (assetInfo)
      s_NeutralNormalMap = assetInfo->m_Data.m_Guid;
    else
      ezLog::Error("Can't find neutral normal map texture {0}", szNeutralNormalMapAssetPath);
  }
  return s_NeutralNormalMap;
}

void ezMaterialAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/ezEditor.NodeGraph");
}

bool ezMaterialAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_sMimeType) const
{
  out_sMimeType = "application/ezEditor.NodeGraph";

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezMaterialAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezMaterialAssetPropertiesPatch_1_2 : public ezGraphPatch
{
public:
  ezMaterialAssetPropertiesPatch_1_2()
    : ezGraphPatch("ezMaterialAssetProperties", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Shader Mode", "ShaderMode");
    pNode->RenameProperty("Base Material", "BaseMaterial");
  }
};

ezMaterialAssetPropertiesPatch_1_2 g_ezMaterialAssetPropertiesPatch_1_2;


class ezMaterialAssetPropertiesPatch_2_3 : public ezGraphPatch
{
public:
  ezMaterialAssetPropertiesPatch_2_3()
    : ezGraphPatch("ezMaterialAssetProperties", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pBaseMatProp = pNode->FindProperty("BaseMaterial");
    auto* pShaderModeProp = pNode->FindProperty("ShaderMode");
    if (pBaseMatProp && pBaseMatProp->m_Value.IsA<ezString>())
    {
      if (!pBaseMatProp->m_Value.Get<ezString>().IsEmpty())
      {
        // BaseMaterial is set
        pNode->ChangeProperty("ShaderMode", (ezInt32)ezMaterialShaderMode::BaseMaterial);
      }
      else
      {
        pNode->ChangeProperty("ShaderMode", (ezInt32)ezMaterialShaderMode::File);
      }
    }
  }
};

ezMaterialAssetPropertiesPatch_2_3 g_ezMaterialAssetPropertiesPatch_2_3;
