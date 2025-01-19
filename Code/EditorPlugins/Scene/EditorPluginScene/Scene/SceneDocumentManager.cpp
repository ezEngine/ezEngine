#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentManager, 1, ezRTTIDefaultAllocator<ezSceneDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezSceneDocumentManager::ezSceneDocumentManager()
{
  // Document type descriptor for a standard EZ scene
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Scene";
    docTypeDesc.m_sFileExtension = "ezScene";
    docTypeDesc.m_sIcon = ":/AssetIcons/Scene.svg";
    docTypeDesc.m_sAssetCategory = "Construction";
    docTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezScene2Document>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene");

    docTypeDesc.m_sResourceFileExtension = "ezBinScene";
    docTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a prefab
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();

    docTypeDesc.m_sDocumentTypeName = "Prefab";
    docTypeDesc.m_sFileExtension = "ezPrefab";
    docTypeDesc.m_sIcon = ":/AssetIcons/Prefab.svg";
    docTypeDesc.m_sAssetCategory = "Construction";
    docTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezSceneDocument>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Prefab");

    docTypeDesc.m_sResourceFileExtension = "ezBinPrefab";
    docTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a layer (similar to a normal scene) as it holds a scene object graph
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Layer";
    docTypeDesc.m_sFileExtension = "ezSceneLayer";
    docTypeDesc.m_sIcon = ":/AssetIcons/Layer.svg";
    docTypeDesc.m_sAssetCategory = "Construction";
    docTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezLayerDocument>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene_Layer");

    docTypeDesc.m_sResourceFileExtension = "";
    // A layer can not be transformed individually (at least at the moment)
    // all layers for a scene are gathered and put into one cohesive runtime scene
    docTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::DisableTransform; // TODO: Disable creation in "New Document"?
  }
}

void ezSceneDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  if (sDocumentTypeName.IsEqual("Scene"))
  {
    out_pDocument = new ezScene2Document(sPath);

    if (bCreateNewDocument)
    {
      SetupDefaultScene(out_pDocument);
    }
  }
  else if (sDocumentTypeName.IsEqual("Prefab"))
  {
    out_pDocument = new ezSceneDocument(sPath, ezSceneDocument::DocumentType::Prefab);
  }
  else if (sDocumentTypeName.IsEqual("Layer"))
  {
    if (pOpenContext == nullptr)
    {
      // Opened individually
      out_pDocument = new ezSceneDocument(sPath, ezSceneDocument::DocumentType::Layer);
    }
    else
    {
      // Opened via a parent scene document
      ezScene2Document* pDoc = const_cast<ezScene2Document*>(ezDynamicCast<const ezScene2Document*>(pOpenContext->GetDocumentObjectManager()->GetDocument()));
      out_pDocument = new ezLayerDocument(sPath, pDoc);
    }
  }
}

void ezSceneDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  for (auto& docTypeDesc : m_DocTypeDescs)
  {
    inout_DocumentTypes.PushBack(&docTypeDesc);
  }
}

void ezSceneDocumentManager::InternalCloneDocument(ezStringView sPath, ezStringView sClonePath, const ezUuid& documentId, const ezUuid& seedGuid, const ezUuid& cloneGuid, ezAbstractObjectGraph* pHeader, ezAbstractObjectGraph* pObjects, ezAbstractObjectGraph* pTypes)
{
  ezAssetDocumentManager::InternalCloneDocument(sPath, sClonePath, documentId, seedGuid, cloneGuid, pHeader, pObjects, pTypes);


  auto pRoot = pObjects->GetNodeByName("ObjectTree");
  ezUuid settingsGuid = pRoot->FindProperty("Settings")->m_Value.Get<ezUuid>();
  auto pSettings = pObjects->GetNode(settingsGuid);
  if (ezRTTI::FindTypeByName(pSettings->GetType()) != ezGetStaticRTTI<ezSceneDocumentSettings>())
    return;

  // Fix up scene layers during cloning
  pObjects->ModifyNodeViaNativeCounterpart(pSettings, [&](void* pNativeObject, const ezRTTI* pType)
    {
    ezSceneDocumentSettings* pObject = static_cast<ezSceneDocumentSettings*>(pNativeObject);

    for (ezSceneLayerBase* pLayerBase : pObject->m_Layers)
    {
      if (auto pLayer = ezDynamicCast<ezSceneLayer*>(pLayerBase))
      {
        if (pLayer->m_Layer == documentId)
        {
          // Fix up main layer reference in layer list
          pLayer->m_Layer = cloneGuid;
        }
        else
        {
          // Clone layer.
          ezStringBuilder sLayerPath;
          {
            auto assetInfo = ezAssetCurator::GetSingleton()->GetSubAsset(pLayer->m_Layer);
            if (assetInfo.isValid())
            {
              sLayerPath = assetInfo->m_pAssetInfo->m_Path;
            }
            else
            {
              ezLog::Error("Failed to resolve layer: {}. Cloned Layer will be invalid.");
              pLayer->m_Layer = ezUuid::MakeInvalid();
            }
          }
          if (!sLayerPath.IsEmpty())
          {
            ezUuid newLayerGuid = pLayer->m_Layer;
            newLayerGuid.CombineWithSeed(seedGuid);

            ezStringBuilder sLayerClonePath = sClonePath;
            sLayerClonePath.RemoveFileExtension();
            sLayerClonePath.Append("_data");
            ezStringBuilder sCloneFleName = ezPathUtils::GetFileNameAndExtension(sLayerPath.GetData());
            sLayerClonePath.AppendPath(sCloneFleName);
            // We assume that all layers are handled by the same document manager, i.e. this.
            CloneDocument(sLayerPath, sLayerClonePath, newLayerGuid).LogFailure();
            pLayer->m_Layer = newLayerGuid;
          }
        }
      }
    } });
}

void ezSceneDocumentManager::SetupDefaultScene(ezDocument* pDocument)
{
  auto history = pDocument->GetCommandHistory();
  history->StartTransaction("Initial Scene Setup");

  const ezUuid skyObjectGuid = ezUuid::MakeUuid();
  const ezUuid lightObjectGuid = ezUuid::MakeUuid();
  const ezUuid meshObjectGuid = ezUuid::MakeUuid();

  // Thumbnail Camera
  {
    const ezUuid objectGuid = ezUuid::MakeUuid();

    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = objectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    // object name
    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Name";
      propCmd.m_NewValue = "Scene Thumbnail Camera";
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    // camera position
    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(0, 0, 0);
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    // camera component
    {
      ezAddObjectCommand cmd;
      cmd.m_Index = -1;
      cmd.SetType("ezCameraComponent");
      cmd.m_Parent = objectGuid;
      cmd.m_sParentProperty = "Components";
      EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

      // camera shortcut
      {
        ezSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "EditorShortcut";
        propCmd.m_NewValue = 1;
        EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
      }

      // camera usage hint
      {
        ezSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "UsageHint";
        propCmd.m_NewValue = (int)ezCameraUsageHint::Thumbnail;
        EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
      }
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = meshObjectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(3, 0, 0);
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = skyObjectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(0, 0, 1);
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      ezRemoveObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index = 0; // There is only one value in the set, CastShadow.
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      ezInsertObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index = 0;
      propCmd.m_NewValue = "SkyLight";
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = lightObjectGuid;
    cmd.m_sParentProperty = "Children";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = ezVec3(0, 0, 2);
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      ezQuat qRot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(55), ezAngle::MakeFromDegree(90));

      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalRotation";
      propCmd.m_NewValue = qRot;
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezSkyBoxComponent");
    cmd.m_Parent = skyObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "CubeMap";
      propCmd.m_NewValue = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "ExposureBias";
      propCmd.m_NewValue = 1.0f;
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezSkyLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezDirectionalLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");
  }

  {
    ezAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("ezMeshComponent");
    cmd.m_Parent = meshObjectGuid;
    cmd.m_sParentProperty = "Components";
    EZ_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

    {
      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Mesh";
      propCmd.m_NewValue = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Base/Meshes/Sphere.ezMeshAsset
      EZ_VERIFY(history->AddCommand(propCmd).Succeeded(), "AddCommand failed");
    }
  }

  history->FinishTransaction();
}
