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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetProperties, 1, ezRTTIDefaultAllocator<ezMaterialAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Base Material", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("Shader", GetShader, SetShader)->AddAttributes(new ezFileBrowserAttribute("Select Shader", "*.ezShader")),
    // This property holds the phantom shader properties type so it is only used in the object graph but not actually in the instance of this object.
    EZ_ACCESSOR_PROPERTY("ShaderProperties", GetShaderProperties, SetShaderProperties)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezContainerAttribute(false, false, false)),

    EZ_MEMBER_PROPERTY("Permutations", m_sPermutationVarValues),
    EZ_MEMBER_PROPERTY("Diffuse Texture", m_sTextureDiffuse)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("Mask Texture", m_sTextureMask)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("Normal Map", m_sTextureNormal)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
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

  static ezHashTable<ezString, PermutationVarConfig> s_PermutationVarConfigs;

  void FillPermutationType(ezShaderParser::ParameterDefinition& def)
  {
    EZ_ASSERT_DEV(def.m_sType.IsEqual("Permutation"), "");

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(def.m_sName, pConfig))
    {
      ezStringBuilder sTemp;
      sTemp.Format("Shaders/PermutationVars/%s.ezPermVar", def.m_sName.GetData());

      ezString sPath = sTemp;
      ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);

      ezFileReader file;
      if (file.Open(sPath).Failed())
      {
        return;
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
      }
    }

    if (pConfig != nullptr)
    {
      def.m_pType = pConfig->m_pType;
    }
  }
}


void ezMaterialAssetProperties::SetBaseMaterial(const char* szBaseMaterial)
{
  m_sBaseMaterial = szBaseMaterial;
  UpdateShader();
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
  UpdateShader();
}


void ezMaterialAssetProperties::UpdateShader()
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

      if (sShaderPath != pType->GetTypeName())
      {
        // Shader has changed, delete old and create new one.
        DeleteProperties();
        CreateProperties(sShaderPath);
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

void ezMaterialAssetProperties::CreateProperties(const char* szShaderPath)
{
  ezCommandHistory* pHistory = m_pDocument->GetCommandHistory();

  const ezRTTI* pType = ezRTTI::FindTypeByName(szShaderPath);

  if (!pType)
  {
    pType = UpdateShaderType(szShaderPath);
  }

  if (pType)
  {
    ezAddObjectCommand cmd;
    cmd.m_pType = pType;
    cmd.m_sParentProperty = "ShaderProperties";
    cmd.m_Parent = m_pDocument->GetPropertyObject()->GetGuid();

    auto res = pHistory->AddCommand(cmd);
    EZ_ASSERT_DEV(res.m_Result.Succeeded(), "Addition of new properties should never fail.");
    LoadOldValues();
  }
}

void ezMaterialAssetProperties::SaveOldValues()
{
  // TODO: Cache old properties so they can be migrated to the new shader's properties.
}

void ezMaterialAssetProperties::LoadOldValues()
{
  // TODO:
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
    if (parameter.m_pType == nullptr && parameter.m_sType.IsEqual("Permutation"))
    {
      FillPermutationType(parameter);
    }
    
    if (parameter.m_pType == nullptr)
    {
      continue;
    }

    ezBitflags<ezPropertyFlags> flags = ezPropertyFlags::Phantom;
    if (parameter.m_pType->IsDerivedFrom<ezEnumBase>())
      flags |= ezPropertyFlags::IsEnum;
    if (parameter.m_pType->IsDerivedFrom<ezBitflagsBase>())
      flags |= ezPropertyFlags::Bitflags;
    if (ezReflectionUtils::IsBasicType(parameter.m_pType))
      flags |= ezPropertyFlags::StandardType;

    ezReflectedPropertyDescriptor propDesc(ezPropertyCategory::Member, parameter.m_sName, parameter.m_pType->GetTypeName(), flags);

    for (auto attribute : parameter.m_Attributes)
    {
      propDesc.m_Attributes.PushBack(attribute);
    }

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
  // The above command may patch the doc with the newest shader properties so we need to clear the undo history here.
  GetCommandHistory()->ClearUndoHistory();
  SetModified(false);
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

void ezMaterialAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  if (!pProp->m_sBaseMaterial.IsEmpty())
    pInfo->m_FileDependencies.Insert(pProp->m_sBaseMaterial);

  if (!pProp->m_sShader.IsEmpty())
    pInfo->m_FileDependencies.Insert(pProp->m_sShader);

  if (!pProp->m_sTextureDiffuse.IsEmpty() && !ezPathUtils::HasExtension(pProp->m_sTextureDiffuse, "color"))
    pInfo->m_FileDependencies.Insert(pProp->m_sTextureDiffuse);

  if (!pProp->m_sTextureMask.IsEmpty() && !ezPathUtils::HasExtension(pProp->m_sTextureMask, "color"))
    pInfo->m_FileDependencies.Insert(pProp->m_sTextureMask);

  if (!pProp->m_sTextureNormal.IsEmpty() && !ezPathUtils::HasExtension(pProp->m_sTextureNormal, "color"))
    pInfo->m_FileDependencies.Insert(pProp->m_sTextureNormal);
}

ezStatus ezMaterialAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform)
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  // see if we can generate a thumbnail
  ezStringBuilder sImageFile = pProp->m_sTextureDiffuse;

  if (sImageFile.IsEmpty())
    sImageFile = pProp->m_sTextureNormal;

  if (sImageFile.IsEmpty())
    sImageFile = pProp->m_sTextureMask;

  if (!sImageFile.IsEmpty())
  {
    ezImage image;
    bool bValidImage = false;

    if (ezConversionUtils::IsStringUuid(sImageFile))
    {
      ezUuid guid = ezConversionUtils::ConvertStringToUuid(sImageFile);

      sImageFile = ezAssetCurator::GetSingleton()->GetAssetInfo(guid)->m_sAbsolutePath;
    }


    if (!ezPathUtils::HasExtension(sImageFile, "color"))
    {
      sImageFile.MakeCleanPath();

      ezString sAbsPath = sImageFile;

      if (!sImageFile.IsAbsolutePath())
      {
        ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath);
      }

      bValidImage = image.LoadFrom(sAbsPath).Succeeded();
    }
    else
    {
      ezStringBuilder sColorName = sImageFile.GetFileName();

      bool bValidColor = false;
      const ezColorGammaUB color = ezConversionUtils::GetColorByName(sColorName, &bValidColor);

      if (!bValidColor)
      {
        ezLog::Error("Material Asset uses an invalid color name '%s'", sColorName.GetData());
      }

      bValidImage = true;
      image.SetWidth(4);
      image.SetHeight(4);
      image.SetDepth(1);
      image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM_SRGB);
      image.SetNumMipLevels(1);
      image.SetNumFaces(1);
      image.AllocateImageData();
      ezUInt8* pPixels = image.GetPixelPointer<ezUInt8>();

      for (ezUInt32 px = 0; px < 4 * 4 * 4; px += 4)
      {
        pPixels[px + 0] = color.r;
        pPixels[px + 1] = color.g;
        pPixels[px + 2] = color.b;
        pPixels[px + 3] = color.a;
      }
    }

    if (bValidImage)
      SaveThumbnail(image);
  }

  // now generate the .ezMaterialBin file
  {
    ezUInt8 uiVersion = 1;

    stream << uiVersion;
    stream << pProp->m_sBaseMaterial;
    stream << pProp->m_sShader;

    // write out the permutation variables
    {
      ezStringBuilder sValues = pProp->m_sPermutationVarValues;

      ezHybridArray<ezString, 16> sVars;
      sValues.Split(false, sVars, ";");

      const ezUInt16 uiPermVars = sVars.GetCount();
      stream << uiPermVars;

      ezStringBuilder sValue;
      ezHybridArray<ezString, 16> sAssignment;
      for (ezString sVar : sVars)
      {
        sValue = sVar;
        sValue.Split(false, sAssignment, "=");

        if (sAssignment.GetCount() == 2)
        {
          stream << sAssignment[0];
          stream << sAssignment[1];
        }
        else
        {
          stream << "";
          stream << "";
        }
      }
    }

    // write out the textures
    {
      ezUInt16 uiTextures = 0;

      if (!pProp->m_sTextureDiffuse.IsEmpty())
        ++uiTextures;
      if (!pProp->m_sTextureMask.IsEmpty())
        ++uiTextures;
      if (!pProp->m_sTextureNormal.IsEmpty())
        ++uiTextures;

      stream << uiTextures;

      if (!pProp->m_sTextureDiffuse.IsEmpty())
      {
        stream << "TexDiffuse";
        stream << pProp->m_sTextureDiffuse;
      }

      if (!pProp->m_sTextureMask.IsEmpty())
      {
        stream << "TexAlphaMask";
        stream << pProp->m_sTextureMask;
      }

      if (!pProp->m_sTextureNormal.IsEmpty())
      {
        stream << "TexNormal";
        stream << pProp->m_sTextureNormal;
      }
    }
  }

  return ezStatus(EZ_SUCCESS);
}
