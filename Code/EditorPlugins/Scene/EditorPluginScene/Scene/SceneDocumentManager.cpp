#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentManager, 1, ezRTTIDefaultAllocator<ezSceneDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezSceneDocumentManager::ezSceneDocumentManager()
{
  {
    m_DocTypeDesc.m_sDocumentTypeName = "Scene";
    m_DocTypeDesc.m_sFileExtension = "ezScene";
    m_DocTypeDesc.m_sIcon = ":/AssetIcons/Scene.png";
    m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezSceneDocument>();
    m_DocTypeDesc.m_pManager = this;

    m_DocTypeDesc.m_sResourceFileExtension = "ezObjectGraph";
    m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::SupportsThumbnail;
  }

  {
    m_DocTypeDesc2.m_sDocumentTypeName = "Prefab";
    m_DocTypeDesc2.m_sFileExtension = "ezPrefab";
    m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Prefab.png";
    m_DocTypeDesc2.m_pDocumentType = ezGetStaticRTTI<ezSceneDocument>();
    m_DocTypeDesc2.m_pManager = this;

    m_DocTypeDesc2.m_sResourceFileExtension = "ezObjectGraph";
    m_DocTypeDesc2.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
  }
}

void ezSceneDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
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
}

void ezSceneDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
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
      propCmd.m_NewValue = ezVec3(0, 0, 0);
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
      propCmd.m_NewValue = ezVec3(3, 0, 0);
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
      propCmd.m_NewValue = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Base/Meshes/Sphere.ezMeshAsset
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  history->FinishTransaction();
}
