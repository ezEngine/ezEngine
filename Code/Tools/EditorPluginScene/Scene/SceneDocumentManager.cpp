#include <PCH.h>

#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Foundation/IO/ChunkStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentManager, 1, ezRTTIDefaultAllocator<ezSceneDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSceneDocumentManager* ezSceneDocumentManager::s_pSingleton = nullptr;


ezSceneDocumentManager::ezSceneDocumentManager()
{
  s_pSingleton = this;

  {
    m_SceneDesc.m_bCanCreate = true;
    m_SceneDesc.m_sDocumentTypeName = "Scene";
    m_SceneDesc.m_sFileExtension = "ezScene";
    m_SceneDesc.m_sIcon = ":/AssetIcons/Scene.png";
    m_SceneDesc.m_pDocumentType = ezGetStaticRTTI<ezSceneDocument>();
    m_SceneDesc.m_pManager = this;
  }

  {
    m_PrefabDesc.m_bCanCreate = true;
    m_PrefabDesc.m_sDocumentTypeName = "Prefab";
    m_PrefabDesc.m_sFileExtension = "ezPrefab";
    m_PrefabDesc.m_sIcon = ":/AssetIcons/Prefab.png";
    m_PrefabDesc.m_pDocumentType = ezGetStaticRTTI<ezSceneDocument>();
    m_PrefabDesc.m_pManager = this;
  }

  // if scene thumbnails are desired, this needs to be removed
  // ezQtImageCache::GetSingleton()->RegisterTypeImage("Scene", QPixmap(":/AssetIcons/Scene.png"));
}


ezBitflags<ezAssetDocumentFlags> ezSceneDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  if (pDescriptor == &m_PrefabDesc)
  {
    return ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
  }
  else
  {
    // if scene thumbnails are desired, this needs to be added
    return ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::SupportsThumbnail;
  }
}

ezStatus ezSceneDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
                                                        ezDocument*& out_pDocument)
{
  ezStatus status;

  if (ezStringUtils::IsEqual(szDocumentTypeName, "Scene"))
  {
    out_pDocument = new ezSceneDocument(szPath, false);

    if (bCreateNewDocument)
    {
      SetupDefaultScene(out_pDocument);
    }
  }
  else if (ezStringUtils::IsEqual(szDocumentTypeName, "Prefab"))
  {
    out_pDocument = new ezSceneDocument(szPath, true);
  }
  else
  {
    status.m_sMessage = "Unknown Document Type";
  }

  if (out_pDocument)
  {
    status.m_Result = EZ_SUCCESS;
    // out_pDocument->SetFilePath(szPath);
  }

  return status;
}

void ezSceneDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_SceneDesc);
  inout_DocumentTypes.PushBack(&m_PrefabDesc);
}

ezString ezSceneDocumentManager::GetResourceTypeExtension(const char* szDocumentPath) const
{
  return "ezObjectGraph";
}

void ezSceneDocumentManager::QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const
{
  inout_AssetTypeNames.Insert("Scene");
  inout_AssetTypeNames.Insert("Prefab");
}

void ezSceneDocumentManager::SetupDefaultScene(ezDocument* pDocument)
{
  auto history = pDocument->GetCommandHistory();
  history->StartTransaction("Initial Scene Setup");

  ezUuid skyObjectGuid;
  skyObjectGuid.CreateNewUuid();
  ezUuid lightObjectGuid;
  lightObjectGuid.CreateNewUuid();
  ezUuid meshObjectGuid;
  meshObjectGuid.CreateNewUuid();

  // Thumbnail Camera
  {
    ezUuid objectGuid;
    objectGuid.CreateNewUuid();

    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = objectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    // object name
    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Name";
      propCmd.m_NewValue = "Scene Thumbnail Camera";
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    // camera position
    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(-3, 0, 2);
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    // camera component
    {
      ezAddObjectCommand cmd;
      cmd.m_Index = -1;
      cmd.SetType("ezCameraComponent");
      cmd.m_Parent = objectGuid;
      cmd.m_sParentProperty = "Components";
      EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      // camera shortcut
      {
        ezSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "EditorShortcut";
        propCmd.m_NewValue = 1;
        EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
      }

      // camera usage hint
      {
        ezSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "UsageHint";
        propCmd.m_NewValue = (int)ezCameraUsageHint::Thumbnail;
        EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
      }
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = meshObjectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(0, 0, -0.5f);
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = skyObjectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(0, 0, 1);
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = lightObjectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(0, 0, 2);
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      ezQuat qRot;
      qRot.SetFromEulerAngles(ezAngle::Degree(0), ezAngle::Degree(55), ezAngle::Degree(90));

      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalRotation";
      propCmd.m_NewValue = qRot;
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezSkyBoxComponent");
    cmd.m_Parent = skyObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "CubeMap";
      propCmd.m_NewValue = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "ExposureBias";
      propCmd.m_NewValue = 1.0f;
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezAmbientLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezDirectionalLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezMeshComponent");
    cmd.m_Parent = meshObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Mesh";
      propCmd.m_NewValue = "{ 87012036-2712-42af-80ba-c0fd29d5a480 }";
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  history->FinishTransaction();
}

