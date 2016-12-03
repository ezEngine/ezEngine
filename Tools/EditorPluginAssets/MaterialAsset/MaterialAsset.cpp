#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <VisualShader/VsCodeGenerator.h>
#include <ToolsFoundation/Command/NodeCommands.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMaterialShaderMode, 1)
EZ_ENUM_CONSTANTS(ezMaterialShaderMode::BaseMaterial, ezMaterialShaderMode::File, ezMaterialShaderMode::Custom)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetProperties, 2, ezRTTIDefaultAllocator<ezMaterialAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("ShaderMode", ezMaterialShaderMode, GetShaderMode, SetShaderMode),
    EZ_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("Shader", GetShader, SetShader)->AddAttributes(new ezFileBrowserAttribute("Select Shader", "*.ezShader")),
    // This property holds the phantom shader properties type so it is only used in the object graph but not actually in the instance of this object.
    EZ_ACCESSOR_PROPERTY("ShaderProperties", GetShaderProperties, SetShaderProperties)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

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

  /// \todo Set proper shader:

  switch (m_ShaderMode)
  {
  case ezMaterialShaderMode::BaseMaterial:
    /// \todo Reset to 'default' (ie from base)
    break;

  case ezMaterialShaderMode::Custom:
    /// \todo Make sure this is correct
    if (m_pDocument) // if m_pDocument == nullptr, we are deserializing
    {
      m_sShader = GetFinalShader();
    }
    break;
  }

  UpdateShader();
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

  // If we update due to the doc being loaded we are not in a transaction so we open one here
  // the undo stack will be cleared after loading to our patch-up won't show up in the undo history.
  bool bOpenedTransaction = false;
  if (!pHistory->IsInTransaction())
  {
    bOpenedTransaction = true;
    pHistory->StartTransaction("Update Material Shader");
  }

  ezDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();

  // TODO: If m_sShader is empty, we need to get the shader of our base material and use that one instead
  // for the code below. The type name is the clean path to the shader at the moment.
  ezStringBuilder sShaderPath = GetFinalShader();
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

  if (bOpenedTransaction)
  {
    pHistory->FinishTransaction();
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

  if (pType)
  {
    ezAddObjectCommand cmd;
    cmd.m_pType = pType;
    cmd.m_sParentProperty = "ShaderProperties";
    cmd.m_Parent = m_pDocument->GetPropertyObject()->GetGuid();
    cmd.m_NewObjectGuid = cmd.m_Parent;
    cmd.m_NewObjectGuid.CombineWithSeed(ezUuid::StableUuidForString("ShaderProperties"));

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
    ezHybridArray<ezAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (ezAbstractProperty* pProp : properties)
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
    ezHybridArray<ezAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (ezAbstractProperty* pProp : properties)
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
            pHistory->AddCommand(cmd);
          }
        }
      }
    }
  }
}

ezString ezMaterialAssetProperties::GetAutoGenShaderPathAbs() const
{
  ezStringBuilder sPath = m_pDocument->GetDocumentPath();
  ezStringBuilder sFilename = sPath.GetFileName();

  sFilename.Append(".autogen.ezShader");
  sPath.ChangeFileNameAndExtension(sFilename);

  return sPath;
}

void ezMaterialAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezMaterialAssetProperties"))
  {
    ezInt64 shaderMode = e.m_pObject->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    //if (shaderMode == ezMaterialShaderMode::File)
    //  props["Shader"].m_Visibility = ezPropertyUiState::Default;
    //else
    //  props["Shader"].m_Visibility = ezPropertyUiState::Invisible;

    //if (shaderMode == ezMaterialShaderMode::BaseMaterial)
    //  props["BaseMaterial"].m_Visibility = ezPropertyUiState::Default;
    //else
    //  props["BaseMaterial"].m_Visibility = ezPropertyUiState::Invisible;
  }
}

ezString ezMaterialAssetProperties::GetFinalShader() const
{
  /// \todo Get rid of this function

  if (m_ShaderMode == ezMaterialShaderMode::File)
    return m_sShader;

  if (m_ShaderMode == ezMaterialShaderMode::Custom)
  {
    ezString sResult = GetAutoGenShaderPathAbs();
    ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sResult);

    return sResult;
  }

  //if (m_ShaderMode == ezMaterialShaderMode::BaseMaterial)
  //{
  //  return m_sShader;
  //}

  return m_sShader;
}

//////////////////////////////////////////////////////////////////////////

ezMaterialAssetDocument::ezMaterialAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezMaterialAssetProperties>(EZ_DEFAULT_NEW(ezMaterialObjectManager), szDocumentPath, true)
{
}

void ezMaterialAssetDocument::InitializeAfterLoading()
{
  ezSimpleAssetDocument<ezMaterialAssetProperties>::InitializeAfterLoading();
  GetProperties()->SetDocument(this);

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
  auto pAssetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(szBaseMaterial);
  if (pAssetInfo == nullptr)
  {
    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pObject);
    UnlinkPrefabs(sel);
  }
  else
  {
    const ezStringBuilder& sNewBase = ezPrefabCache::GetSingleton()->GetCachedPrefabDocument(pAssetInfo->m_Info.m_DocumentID);
    const ezAbstractObjectGraph* pBaseGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(pAssetInfo->m_Info.m_DocumentID);

    ezUuid seed = GetSeedFromBaseMaterial(pBaseGraph);
    if (sNewBase.IsEmpty() || !pBaseGraph || !seed.IsValid())
    {
      ezLog::Error("The selected base material '%s' is not a valid material file!", szBaseMaterial);
      return;
    }

    {
      auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(pObject->GetGuid());

      if (pMeta->m_CreateFromPrefab != pAssetInfo->m_Info.m_DocumentID)
      {
        pMeta->m_sBasePrefab = sNewBase;
        pMeta->m_CreateFromPrefab = pAssetInfo->m_Info.m_DocumentID;
        pMeta->m_PrefabSeedGuid = seed;
      }
      m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
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
    //Create seed that converts base guid into instance guid
    instanceGuid.RevertCombinationWithSeed(baseGuid);
    return instanceGuid;
  }

  return ezUuid();
}

ezUuid ezMaterialAssetDocument::GetMaterialNodeGuid(const ezAbstractObjectGraph& graph)
{
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    if (ezStringUtils::IsEqual(it.Value()->GetType(), ezGetStaticRTTI<ezMaterialAssetProperties>()->GetTypeName()))
    {
      return it.Value()->GetGuid();
    }
  }
  return ezUuid();
}

void ezMaterialAssetDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab)
{
  // Base
  ezAbstractObjectGraph baseGraph;
  ezPrefabUtils::LoadGraph(baseGraph, szBasePrefab);
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
    ezString sGuid = ezConversionUtils::ToString(PrefabAsset);
    ezLog::Error("Can't update prefab, new base graph does not exist: %s", sGuid.GetData());
    return;
  }
  leftGraph.PruneGraph(GetMaterialNodeGuid(leftGraph));

  // Instance
  ezAbstractObjectGraph rightGraph;
  {
    ezDocumentObjectConverterWriter writer(&rightGraph, pObject->GetDocumentObjectManager(), true, true);
    writer.AddObjectToGraph(pObject);
    rightGraph.ReMapNodeGuids(PrefabSeed, true);
  }

  // Merge diffs relative to base
  ezDeque<ezAbstractGraphDiffOperation> mergedDiff;
  ezPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);

  // Skip shader changes and add / remove calls, we want to preserve everything from base
  ezDeque<ezAbstractGraphDiffOperation> cleanedDiff;
  for (const ezAbstractGraphDiffOperation& op : mergedDiff)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      if (op.m_sProperty == "Shader" || op.m_sProperty == "ShaderProperties")
        continue;

      cleanedDiff.PushBack(op);
    }
  }

  // Apply diff to base, making it the new instance
  baseGraph.ApplyDiff(cleanedDiff);

  // Create a new diff that changes our current instance to the new instance
  ezDeque<ezAbstractGraphDiffOperation> newInstanceToCurrentInstance;
  baseGraph.CreateDiffWithBaseGraph(rightGraph, newInstanceToCurrentInstance);
  if (false)
  {
    ezFileWriter file;
    file.Open("C:\\temp\\Material - diff.txt");

    ezStringBuilder sDiff;
    sDiff.Append("######## New Instance To Instance #######\n");
    ezPrefabUtils::WriteDiff(newInstanceToCurrentInstance, sDiff);
    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount());
  }
  // Apply diff to current instance
  // Shader needs to be set first
  for (ezUInt32 i = 0; i < newInstanceToCurrentInstance.GetCount(); ++i)
  {
    if (newInstanceToCurrentInstance[i].m_sProperty == "Shader")
    {
      ezAbstractGraphDiffOperation op = newInstanceToCurrentInstance[i];
      newInstanceToCurrentInstance.RemoveAt(i);
      newInstanceToCurrentInstance.Insert(op, 0);
      break;
    }
  }
  for (const ezAbstractGraphDiffOperation& op : newInstanceToCurrentInstance)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      ezSetObjectPropertyCommand cmd;
      cmd.m_Object = op.m_Node;
      cmd.m_Object.CombineWithSeed(PrefabSeed);
      cmd.m_NewValue = op.m_Value;
      cmd.m_sProperty = op.m_sProperty;
      GetCommandHistory()->AddCommand(cmd);
    }
  }

  // Update prefab meta data
  {
    auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(pObject->GetGuid());
    pMeta->m_CreateFromPrefab = PrefabAsset; //Should not change
    pMeta->m_PrefabSeedGuid = PrefabSeed; //Should not change
    pMeta->m_sBasePrefab = sLeft;

    m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }
}

ezStatus ezMaterialAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  EZ_SUCCEED_OR_RETURN(RecreateVisualShaderFile(szPlatform));

  return WriteMaterialAsset(stream, szPlatform);
}

ezStatus ezMaterialAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  return ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
}

void ezMaterialAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  if (pManager->IsNode(pObject))
  {
    auto outputs = pManager->GetOutputPins(pObject);
    for (const ezPin* pPinSource : outputs)
    {
      auto inputs = pPinSource->GetConnections();
      for (const ezConnection* pConnection : inputs)
      {
        const ezPin* pPinTarget = pConnection->GetTargetPin();

        inout_uiHash = ezHashing::MurmurHash64(&pPinSource->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash = ezHashing::MurmurHash64(&pPinTarget->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash = ezHashing::MurmurHash64(pPinSource->GetName(), ezStringUtils::GetStringElementCount(pPinSource->GetName()), inout_uiHash);
        inout_uiHash = ezHashing::MurmurHash64(pPinTarget->GetName(), ezStringUtils::GetStringElementCount(pPinTarget->GetName()), inout_uiHash);
      }
    }
  }
}

void ezMaterialAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezMaterialAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph);
}

void ezMaterialAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_ShaderMode == ezMaterialShaderMode::Custom)
  {
    /// \todo This doesn't work!!!
    // Dependencies are files that have to exist, otherwise asset transform fails
    // But we are actually GENERATING this file! That means when the file does not exist, transform will never be called,
    // if we put this as a dependency...
    // But NOT having it as a dependency somehow also means the asset is never transformed when stuff changes, argh!
    // Basically we are generating multiple outputs here ... and need to check whether any of them is out-of-date (or at least missing).

    // Also the Visual Shader node configuration files would need to be a dependency of the auto-generated shader.

    //pInfo->m_FileDependencies.Insert(GetProperties()->GetFinalShader());
  }
}

ezStatus ezMaterialAssetDocument::WriteMaterialAsset(ezStreamWriter& stream, const char* szPlatform) const
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  // now generate the .ezMaterialBin file
  {
    const ezUInt8 uiVersion = 2;

    stream << uiVersion;
    stream << pProp->m_sBaseMaterial;
    stream << pProp->GetFinalShader();

    ezHybridArray<ezAbstractProperty*, 16> Textures;
    ezHybridArray<ezAbstractProperty*, 16> Permutation;
    ezHybridArray<ezAbstractProperty*, 16> Constants;

    const ezDocumentObject* pObject = GetShaderPropertyObject();
    if (pObject)
    {
      bool hasBaseMaterial = ezPrefabUtils::GetPrefabRoot(pObject, m_DocumentObjectMetaData).IsValid();
      auto pType = pObject->GetTypeAccessor().GetType();
      ezHybridArray<ezAbstractProperty*, 32> properties;
      pType->GetAllProperties(properties);

      for (auto* pProp : properties)
      {
        if (hasBaseMaterial && IsDefaultValue(pObject, pProp->GetPropertyName(), false))
          continue;

        const ezCategoryAttribute* pCategory = pProp->GetAttributeByType<ezCategoryAttribute>();

        EZ_ASSERT_DEBUG(pCategory, "Category cannot be null for a shader property");
        if (pCategory == nullptr)
          continue;

        if (ezStringUtils::IsEqual(pCategory->GetCategory(), "Texture"))
        {
          Textures.PushBack(pProp);
        }
        else if (ezStringUtils::IsEqual(pCategory->GetCategory(), "Permutation"))
        {
          Permutation.PushBack(pProp);
        }
        else if (ezStringUtils::IsEqual(pCategory->GetCategory(), "Constant"))
        {
          Constants.PushBack(pProp);
        }
        else
        {
          EZ_REPORT_FAILURE("Invalid shader property type '%s'", pCategory->GetCategory());
        }
      }
    }

    // write out the permutation variables
    {
      const ezUInt16 uiPermVars = Permutation.GetCount();
      stream << uiPermVars;

      for (ezUInt32 p = 0; p < uiPermVars; ++p)
      {
        ezString sValue;

        const char* szName = Permutation[p]->GetPropertyName();
        if (Permutation[p]->GetSpecificType()->GetVariantType() == ezVariantType::Bool)
        {
          sValue = pObject->GetTypeAccessor().GetValue(szName).Get<bool>() ? "TRUE" : "FALSE";
        }
        else if (Permutation[p]->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        {
          ezStringBuilder s;
          ezReflectionUtils::EnumerationToString(Permutation[p]->GetSpecificType(), pObject->GetTypeAccessor().GetValue(szName).ConvertTo<ezInt64>(), s);

          sValue = s.FindLastSubString("::") + 2;
        }
        else
        {
          EZ_REPORT_FAILURE("Invalid shader permutation property type '%s'", Permutation[p]->GetSpecificType()->GetTypeName());
        }

        stream << szName;
        stream << sValue;
      }
    }

    // write out the textures
    {
      const ezUInt16 uiTextures = Textures.GetCount();
      stream << uiTextures;

      for (ezUInt32 p = 0; p < uiTextures; ++p)
      {
        const char* szName = Textures[p]->GetPropertyName();
        ezString sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<ezString>();

        stream << szName;
        stream << sValue;
      }
    }

    // write out the constants
    {
      const ezUInt16 uiConstants = Constants.GetCount();
      stream << uiConstants;

      for (ezUInt32 p = 0; p < uiConstants; ++p)
      {
        const char* szName = Constants[p]->GetPropertyName();
        ezVariant value = pObject->GetTypeAccessor().GetValue(szName);

        stream << szName;
        stream << value;
      }
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMaterialAssetDocument::RecreateVisualShaderFile(const char* szPlatform)
{
  ezStringBuilder sAutoGenShader = GetProperties()->GetAutoGenShaderPathAbs();

  if (GetProperties()->m_ShaderMode != ezMaterialShaderMode::Custom)
  {
    ezOSFile::DeleteFile(sAutoGenShader);
    return ezStatus(EZ_SUCCESS);
  }

  ezVisualShaderCodeGenerator codeGen;

  EZ_SUCCEED_OR_RETURN(codeGen.GenerateVisualShader(static_cast<ezDocumentNodeManager*>(GetObjectManager()), szPlatform));

  ezFileWriter file;
  if (file.Open(sAutoGenShader).Succeeded())
  {
    ezStringBuilder shader = codeGen.GetFinalShaderCode();

    file.WriteBytes(shader.GetData(), shader.GetElementCount());
    file.Close();

    // This should update the shader parameter section in all affected materials
    ezShaderTypeRegistry::GetSingleton()->GetShaderType(sAutoGenShader);

    return ezStatus(EZ_SUCCESS);
  }
  else
    return ezStatus("Failed to write auto-generated shader to '%s'", sAutoGenShader.GetData());
}

static void MarkReachableNodes(ezMap<const ezDocumentObject*, bool>& AllNodes, const ezDocumentObject* pRoot, ezDocumentNodeManager* pNodeManager)
{
  if (AllNodes[pRoot])
    return;

  AllNodes[pRoot] = true;

  auto allInputs = pNodeManager->GetInputPins(pRoot);

  // we start at the final output, so use the inputs on a node and then walk backwards
  for (ezPin* pTargetPin : allInputs)
  {
    const ezArrayPtr<const ezConnection* const> connections = pTargetPin->GetConnections();

    // all incoming connections at the input pin, there should only be one though
    for (const ezConnection* const pConnection : connections)
    {
      // output pin on other node connecting to this node
      const ezPin* pSourcePin = pConnection->GetSourcePin();

      // recurse from here
      MarkReachableNodes(AllNodes, pSourcePin->GetParent(), pNodeManager);
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

      pHistory->AddCommand(rem);
    }

    pHistory->FinishTransaction();
  }
}

ezUuid ezMaterialAssetDocument::GetLitBaseMaterial()
{
  if (!s_LitBaseMaterial.IsValid())
  {
    static const char* szLitMaterialAssetPath = "Materials/BaseMaterials/Lit.ezMaterialAsset";
    auto assetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(szLitMaterialAssetPath);
    if (assetInfo)
      s_LitBaseMaterial = assetInfo->m_Info.m_DocumentID;
    else
      ezLog::Error("Can't find default lit material %s", szLitMaterialAssetPath);
  }
  return s_LitBaseMaterial;
}

ezUuid ezMaterialAssetDocument::GetLitAlphaTextBaseMaterial()
{
  if (!s_LitAlphaTextBaseMaterial.IsValid())
  {
    static const char* szLitAlphaTestMaterialAssetPath = "Materials/BaseMaterials/LitAlphaTest.ezMaterialAsset";
    auto assetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(szLitAlphaTestMaterialAssetPath);
    if (assetInfo)
      s_LitAlphaTextBaseMaterial = assetInfo->m_Info.m_DocumentID;
    else
      ezLog::Error("Can't find default lit alpha test material %s", szLitAlphaTestMaterialAssetPath);
  }
  return s_LitAlphaTextBaseMaterial;
}

ezUuid ezMaterialAssetDocument::GetNeutralNormalMap()
{
  if (!s_NeutralNormalMap.IsValid())
  {
    static const char* szNeutralNormalMapAssetPath = "Textures/NeutralNormal.ezTextureAsset";
    auto assetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(szNeutralNormalMapAssetPath);
    if (assetInfo)
      s_NeutralNormalMap = assetInfo->m_Info.m_DocumentID;
    else
      ezLog::Error("Can't find neutral normal map texture %s", szNeutralNormalMapAssetPath);
  }
  return s_NeutralNormalMap;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezMaterialAssetPropertiesPatch_1_2 : public ezGraphPatch
{
public:
  ezMaterialAssetPropertiesPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezMaterialAssetProperties>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Shader Mode", "ShaderMode");
    pNode->RenameProperty("Base Material", "BaseMaterial");
  }
};

ezMaterialAssetPropertiesPatch_1_2 g_ezMaterialAssetPropertiesPatch_1_2;


/*
class ezMaterialAssetPropertiesPatch_2_3 : public ezGraphPatch
{
public:
  ezMaterialAssetPropertiesPatch_2_3()
    : ezGraphPatch(ezGetStaticRTTI<ezMaterialAssetProperties>(), 3) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pBaseMatProp = pNode->FindProperty("BaseMaterial");
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
*/