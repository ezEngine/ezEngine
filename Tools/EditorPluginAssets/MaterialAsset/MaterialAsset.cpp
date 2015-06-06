#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjects.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Image/Image.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocument, ezAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialAssetDocument::ezMaterialAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezMaterialAssetProperties, ezMaterialAssetObject, ezMaterialAssetObjectManager>(szDocumentPath)
{
}

void ezMaterialAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  if (!pProp->m_sBaseMaterial.IsEmpty())
    pInfo->m_FileDependencies.PushBack(pProp->m_sBaseMaterial);

  if (!pProp->m_sShader.IsEmpty())
    pInfo->m_FileDependencies.PushBack(pProp->m_sShader);

  if (!pProp->m_sTextureDiffuse.IsEmpty())
    pInfo->m_FileDependencies.PushBack(pProp->m_sTextureDiffuse);

  if (!pProp->m_sTextureMask.IsEmpty())
    pInfo->m_FileDependencies.PushBack(pProp->m_sTextureMask);

  if (!pProp->m_sTextureNormal.IsEmpty())
    pInfo->m_FileDependencies.PushBack(pProp->m_sTextureNormal);
}

ezStatus ezMaterialAssetDocument::InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform)
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
    sImageFile.MakeCleanPath();

    ezString sAbsPath = sImageFile;

    if (!sImageFile.IsAbsolutePath())
    {
      ezEditorApp::GetInstance()->MakeDataDirectoryRelativePathAbsolute(sAbsPath);
    }

    ezImage image;
    if (image.LoadFrom(sAbsPath).Succeeded())
    {
      SaveThumbnail(image);
    }
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

