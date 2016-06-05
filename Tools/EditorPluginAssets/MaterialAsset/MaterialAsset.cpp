#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Image/Image.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetProperties, 1, ezRTTIDefaultAllocator<ezMaterialAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Base Material", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("Shader", GetShader, SetShader)->AddAttributes(new ezFileBrowserAttribute("Select Shader", "*.ezShader")),
    // This property holds the phantom shader properties type so it is only used in the object graph but not actually in the instance of this object.
    EZ_ACCESSOR_PROPERTY("ShaderProperties", GetShaderProperties, SetShaderProperties)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE


namespace
{
  struct PermutationVarConfig
  {
    ezVariant m_DefaultValue;
    const ezRTTI* m_pType;
  };

  static ezHashTable<const char*, const ezRTTI*> s_NameToTypeTable;

  void InitializeTables()
  {
    if (!s_NameToTypeTable.IsEmpty())
      return;

    s_NameToTypeTable.Insert("float", ezGetStaticRTTI<float>());
    s_NameToTypeTable.Insert("float2", ezGetStaticRTTI<ezVec2>());
    s_NameToTypeTable.Insert("float3", ezGetStaticRTTI<ezVec3>());
    s_NameToTypeTable.Insert("float4", ezGetStaticRTTI<ezVec4>());
    s_NameToTypeTable.Insert("Color", ezGetStaticRTTI<ezColor>());
    s_NameToTypeTable.Insert("Texture", ezGetStaticRTTI<ezString>());
    s_NameToTypeTable.Insert("Texture2D", ezGetStaticRTTI<ezString>());
    s_NameToTypeTable.Insert("Texture3D", ezGetStaticRTTI<ezString>());
    s_NameToTypeTable.Insert("TextureCube", ezGetStaticRTTI<ezString>());
  }

  static ezHashTable<ezString, PermutationVarConfig> s_PermutationVarConfigs;

  const ezRTTI* GetPermutationType(ezShaderParser::ParameterDefinition& def)
  {
    EZ_ASSERT_DEV(def.m_sType.IsEqual("Permutation"), "");

    PermutationVarConfig* pConfig = nullptr;
    if (s_PermutationVarConfigs.TryGetValue(def.m_sName, pConfig))
    {
      return pConfig->m_pType;
    }

    ezStringBuilder sTemp;
    sTemp.Format("Shaders/PermutationVars/%s.ezPermVar", def.m_sName.GetData());

    ezString sPath = sTemp;
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);

    ezFileReader file;
    if (file.Open(sPath).Failed())
    {
      return nullptr;
    }

    sTemp.ReadAll(file);

    ezVariant defaultValue;
    ezHybridArray<ezHashedString, 16> enumValues;

    ezShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumValues);
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
        descEnum.m_uiTypeSize = 0;
        descEnum.m_uiTypeVersion = 1;

        ezArrayPtr<ezPropertyAttribute* const> noAttributes;

        ezStringBuilder sEnumName;
        sEnumName.Format("%s::Default", def.m_sName.GetData());

        descEnum.m_Properties.PushBack(ezReflectedPropertyDescriptor(sEnumName, defaultValue.Get<ezUInt32>(), noAttributes));
          
        for (ezUInt32 i = 0; i < enumValues.GetCount(); ++i)
        {
          if (enumValues[i].IsEmpty())
            continue;

          ezStringBuilder sEnumName;
          sEnumName.Format("%s::%s", def.m_sName.GetData(), enumValues[i].GetData());

          descEnum.m_Properties.PushBack(ezReflectedPropertyDescriptor(sEnumName, (ezUInt32)i, noAttributes));
        }

        pConfig->m_pType = ezPhantomRttiManager::RegisterType(descEnum);
      }

      return pConfig->m_pType;
    }

    return nullptr;
  }

  const ezRTTI* GetType(ezShaderParser::ParameterDefinition& def)
  {
    InitializeTables();

    if (def.m_sType.IsEqual("Permutation"))
    {
      return GetPermutationType(def);
    }

    const ezRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(def.m_sType.GetData(), pType);
    return pType;
  }

  void AddAttributes(ezShaderParser::ParameterDefinition& def, ezHybridArray<ezPropertyAttribute*, 2>& attributes)
  {
    if (def.m_sType.StartsWith_NoCase("texture"))
    {
      attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Texture"));
    }
    else if (def.m_sType.StartsWith_NoCase("permutation"))
    {
      attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Permutation"));
    }
    else
    {
      attributes.PushBack(EZ_DEFAULT_NEW(ezCategoryAttribute, "Constant"));
    }

    if (def.m_sType.IsEqual("Texture2D") || def.m_sType.IsEqual("Texture"))
    {
      attributes.PushBack(EZ_DEFAULT_NEW(ezAssetBrowserAttribute, "Texture 2D"));
    }
    else if (def.m_sType.IsEqual("Texture3D"))
    {
      attributes.PushBack(EZ_DEFAULT_NEW(ezAssetBrowserAttribute, "Texture 3D"));
    }
    else if (def.m_sType.IsEqual("TextureCube"))
    {
      attributes.PushBack(EZ_DEFAULT_NEW(ezAssetBrowserAttribute, "Texture Cube"));
    }

    for (auto& attributeDef : def.m_Attributes)
    {
      if (attributeDef.m_sName.IsEqual("Default"))
      {
		//TODO: this needs a proper implementation for types other than float
        double fValue;
        ezConversionUtils::StringToFloat(attributeDef.m_sValue, fValue);

        attributes.PushBack(EZ_DEFAULT_NEW(ezDefaultValueAttribute, fValue));
      }
    }
  }
}


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
  m_sShader = szShader;
  UpdateShader();
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
    pHistory->StartTransaction();
  }

  ezDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();

  // TODO: If m_sShader is empty, we need to get the shader of our base material and use that one instead
  // for the code below. The type name is the clean path to the shader at the moment.
  ezStringBuilder sShaderPath = m_sShader;
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

      if (sShaderPath != pType->GetTypeName() || bForce)
      {
        // Shader has changed, delete old and create new one.
        DeleteProperties();
        CreateProperties(sShaderPath, bForce);
      }
      else
      {
        // Same shader but it could have changed so try to update it anyway.
        UpdateShaderType(sShaderPath);
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

void ezMaterialAssetProperties::CreateProperties(const char* szShaderPath, bool bForce)
{
  ezCommandHistory* pHistory = m_pDocument->GetCommandHistory();

  const ezRTTI* pType = ezRTTI::FindTypeByName(szShaderPath);

  if (!pType || bForce)
  {
    pType = UpdateShaderType(szShaderPath);
  }

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
            cmd.m_sPropertyPath = sPropName;
            cmd.m_NewValue = it.Value();

            // Do not check for success, if a cached value failed to apply, simply ignore it.
            pHistory->AddCommand(cmd);
          }
        }
      }
    }
  }
}

const ezRTTI* ezMaterialAssetProperties::UpdateShaderType(const char* szShaderPath)
{
  ezHybridArray<ezShaderParser::ParameterDefinition, 16> parameters;

  {
    ezString sShaderPath = szShaderPath;
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sShaderPath);
      
    ezFileReader file;
    if (file.Open(sShaderPath).Failed())
    {
      return nullptr;
    }

    ezShaderParser::ParseMaterialParameterSection(file, parameters);
  }

  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = szShaderPath;
  desc.m_sPluginName = "ShaderTypes";
  desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom;
  desc.m_uiTypeSize = 0;
  desc.m_uiTypeVersion = 1;

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

    AddAttributes(parameter, propDesc.m_Attributes);    

    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  const ezRTTI* pShaderType = ezPhantomRttiManager::RegisterType(desc);
  return pShaderType;
}

//////////////////////////////////////////////////////////////////////////

ezMaterialAssetDocument::ezMaterialAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezMaterialAssetProperties>(szDocumentPath)
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

ezBitflags<ezAssetDocumentFlags> ezMaterialAssetDocument::GetAssetFlags() const
{
  return ezAssetDocumentFlags::AutoTransformOnSave;
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

void ezMaterialAssetDocument::SetBaseMaterial(const char* szBaseMaterial)
{
  ezDocumentObject* pObject = GetPropertyObject();
  auto* pAssetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(szBaseMaterial);
  if (pAssetInfo == nullptr)
  {
    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pObject);
    UnlinkPrefabs(sel);
  }
  else
  {
    const ezString& sNewBase = ReadDocumentAsString(pAssetInfo->m_sAbsolutePath);
    ezUuid seed = GetSeedFromBaseMaterial(sNewBase);
    if (!seed.IsValid())
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

ezUuid ezMaterialAssetDocument::GetSeedFromBaseMaterial(const char* szBaseGraph)
{
  ezAbstractObjectGraph baseGraph;
  ezPrefabUtils::LoadGraph(baseGraph, szBaseGraph);

  ezUuid instanceGuid = GetPropertyObject()->GetGuid();
  ezUuid baseGuid = ezMaterialAssetDocument::GetMaterialNodeGuid(baseGraph);
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
  const ezString& sLeft = GetCachedPrefabDocument(PrefabAsset);
  ezAbstractObjectGraph leftGraph;
  ezPrefabUtils::LoadGraph(leftGraph, sLeft);
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
      cmd.m_sPropertyPath = op.m_sProperty;
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

void ezMaterialAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  if (!pProp->m_sBaseMaterial.IsEmpty())
    pInfo->m_FileDependencies.Insert(pProp->m_sBaseMaterial);

  if (!pProp->m_sShader.IsEmpty())
    pInfo->m_FileDependencies.Insert(pProp->m_sShader);
}

ezStatus ezMaterialAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform)
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  //// see if we can generate a thumbnail
  //ezStringBuilder sImageFile = pProp->m_sTextureDiffuse;

  //if (sImageFile.IsEmpty())
  //  sImageFile = pProp->m_sTextureNormal;

  //if (sImageFile.IsEmpty())
  //  sImageFile = pProp->m_sTextureMask;

  //if (!sImageFile.IsEmpty())
  //{
  //  ezImage image;
  //  bool bValidImage = false;

  //  if (ezConversionUtils::IsStringUuid(sImageFile))
  //  {
  //    ezUuid guid = ezConversionUtils::ConvertStringToUuid(sImageFile);

  //    sImageFile = ezAssetCurator::GetSingleton()->GetAssetInfo(guid)->m_sAbsolutePath;
  //  }


  //  if (!ezPathUtils::HasExtension(sImageFile, "color"))
  //  {
  //    sImageFile.MakeCleanPath();

  //    ezString sAbsPath = sImageFile;

  //    if (!sImageFile.IsAbsolutePath())
  //    {
  //      ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath);
  //    }

  //    bValidImage = image.LoadFrom(sAbsPath).Succeeded();
  //  }
  //  else
  //  {
  //    ezStringBuilder sColorName = sImageFile.GetFileName();

  //    bool bValidColor = false;
  //    const ezColorGammaUB color = ezConversionUtils::GetColorByName(sColorName, &bValidColor);

  //    if (!bValidColor)
  //    {
  //      ezLog::Error("Material Asset uses an invalid color name '%s'", sColorName.GetData());
  //    }

  //    bValidImage = true;
  //    image.SetWidth(4);
  //    image.SetHeight(4);
  //    image.SetDepth(1);
  //    image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM_SRGB);
  //    image.SetNumMipLevels(1);
  //    image.SetNumFaces(1);
  //    image.AllocateImageData();
  //    ezUInt8* pPixels = image.GetPixelPointer<ezUInt8>();

  //    for (ezUInt32 px = 0; px < 4 * 4 * 4; px += 4)
  //    {
  //      pPixels[px + 0] = color.r;
  //      pPixels[px + 1] = color.g;
  //      pPixels[px + 2] = color.b;
  //      pPixels[px + 3] = color.a;
  //    }
  //  }

  //  if (bValidImage)
  //    SaveThumbnail(image);
  //}

  // now generate the .ezMaterialBin file
  {
    ezUInt8 uiVersion = 2;

    stream << uiVersion;
    stream << pProp->m_sBaseMaterial;
    stream << pProp->m_sShader;

    ezHybridArray<ezAbstractProperty*, 16> Textures;
    ezHybridArray<ezAbstractProperty*, 16> Permutation;
    ezHybridArray<ezAbstractProperty*, 16> Constants;

    ezDocumentObject* pObject = GetShaderPropertyObject();
    if (pObject)
    {
      bool hasBaseMaterial = ezPrefabUtils::GetPrefabRoot(pObject, m_DocumentObjectMetaData).IsValid();
      auto pType = pObject->GetTypeAccessor().GetType();
      ezHybridArray<ezAbstractProperty*, 32> properties;
      pType->GetAllProperties(properties);

      for (auto* pProp : properties)
      {
        if (hasBaseMaterial && IsDefaultValue(pObject, pProp->GetPropertyName()))
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
