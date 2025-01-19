#include <EditorTest/EditorTestPCH.h>

#include "EditorFramework/Assets/AssetBrowserWidget.moc.h"
#include "Misc.h"
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static ezEditorTestMisc s_EditorTestMisc;

const char* ezEditorTestMisc::GetTestName() const
{
  return "Misc Tests";
}

void ezEditorTestMisc::SetupSubTests()
{
  AddSubTest("GameObject References", SubTests::GameObjectReferences);
  AddSubTest("Default Values", SubTests::DefaultValues);
  AddSubTest("Asset Browser Model", SubTests::AssetBrowerModel);
}

ezResult ezEditorTestMisc::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return EZ_FAILURE;

  if (ezStatus res = ezAssetCurator::GetSingleton()->TransformAllAssets(); res.Failed())
  {
    ezLog::Error("Asset transform failed: {}", res.GetMessageString());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezEditorTestMisc::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  ezMemoryTracker::DumpMemoryLeaks();

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestMisc::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::GameObjectReferences:
      return GameObjectReferencesTest();

    case SubTests::DefaultValues:
      return DefaultValuesTest();

    case SubTests::AssetBrowerModel:
      return AssetBrowerModelTest();
  }

  // const auto& allDesc = ezDocumentManager::GetAllDocumentDescriptors();
  // for (auto* pDesc : allDesc)
  //{
  //  if (pDesc->m_bCanCreate)
  //  {
  //    ezStringBuilder sName = m_sProjectPath;
  //    sName.AppendPath(pDesc->m_sDocumentTypeName);
  //    sName.ChangeFileExtension(pDesc->m_sFileExtension);
  //    ezDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow);
  //    EZ_TEST_BOOL(pDoc);
  //    ProcessEvents();
  //  }
  //}
  //// Make sure the engine process did not crash after creating every kind of document.
  // EZ_TEST_BOOL(!ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  ////TODO: Newly created assets actually do not transform cleanly.
  // if (false)
  //{
  //  ezAssetCurator::GetSingleton()->TransformAllAssets();

  //  ezUInt32 uiNumAssets;
  //  ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> sections;
  //  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  //  EZ_TEST_INT(sections[ezAssetInfo::TransformState::TransformError], 0);
  //  EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingDependency], 0);
  //  EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingReference], 0);
  //}
  return ezTestAppRun::Quit;
}

ezResult ezEditorTestMisc::InitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezResult ezEditorTestMisc::DeInitializeSubTest(ezInt32 iIdentifier)
{
  ezDocumentManager::CloseAllDocuments();
  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestMisc::GameObjectReferencesTest()
{
  m_pDocument = SUPER::OpenDocument("Scenes/GameObjectReferences.ezScene");

  if (!EZ_TEST_BOOL(m_pDocument != nullptr))
    return ezTestAppRun::Quit;

  EZ_ANALYSIS_ASSUME(m_pDocument != nullptr);
  ezAssetCurator::GetSingleton()->TransformAsset(m_pDocument->GetGuid(), ezTransformFlags::Default);

  ezQtEngineDocumentWindow* pWindow = qobject_cast<ezQtEngineDocumentWindow*>(ezQtDocumentWindow::FindWindowByDocument(m_pDocument));

  if (!EZ_TEST_BOOL(pWindow != nullptr))
    return ezTestAppRun::Quit;

  EZ_ANALYSIS_ASSUME(pWindow != nullptr);
  auto viewWidgets = pWindow->GetViewWidgets();

  if (!EZ_TEST_BOOL(!viewWidgets.IsEmpty()))
    return ezTestAppRun::Quit;

  ezQtEngineViewWidget::InteractionContext ctxt;
  ctxt.m_pLastHoveredViewWidget = viewWidgets[0];
  ezQtEngineViewWidget::SetInteractionContext(ctxt);

  viewWidgets[0]->m_pViewConfig->m_RenderMode = ezViewRenderMode::Default;
  viewWidgets[0]->m_pViewConfig->m_Perspective = ezSceneViewPerspective::Perspective;
  viewWidgets[0]->m_pViewConfig->ApplyPerspectiveSetting(90.0f);

  ExecuteDocumentAction("Scene.Camera.JumpTo.0", m_pDocument, true);

  for (int i = 0; i < 10; ++i)
  {
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    ProcessEvents();
  }

  EZ_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

  EZ_TEST_LINE_IMAGE(1, 100);

  // Move everything to the layer and repeat the test.
  ezScene2Document* pScene = ezDynamicCast<ezScene2Document*>(m_pDocument);
  ezHybridArray<ezUuid, 2> layerGuids;
  pScene->GetAllLayers(layerGuids);
  EZ_TEST_INT(layerGuids.GetCount(), 2);
  ezUuid layerGuid = layerGuids[0] == pScene->GetGuid() ? layerGuids[1] : layerGuids[0];

  auto pAccessor = pScene->GetObjectAccessor();
  auto pRoot = pScene->GetObjectManager()->GetRootObject();
  ezHybridArray<ezVariant, 16> values;
  pAccessor->GetValuesByName(pRoot, "Children", values).AssertSuccess();

  ezDeque<const ezDocumentObject*> assets;
  for (auto& value : values)
  {
    assets.PushBack(pAccessor->GetObject(value.Get<ezUuid>()));
  }
  ezDeque<const ezDocumentObject*> newObjects;
  MoveObjectsToLayer(pScene, assets, layerGuid, newObjects);

  EZ_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

  EZ_TEST_LINE_IMAGE(1, 100);

  return ezTestAppRun::Quit;
}

bool CheckDefaultValue(const ezAbstractProperty* pProperty, const ezDefaultValueAttribute* pAttrib)
{
  ezVariant defaultValue = pAttrib->GetValue();

  const bool isValueType = ezReflectionUtils::IsValueType(pProperty);
  const ezVariantType::Enum type = pProperty->GetFlags().IsSet(ezPropertyFlags::Pointer) || (pProperty->GetFlags().IsSet(ezPropertyFlags::Class) && !isValueType) ? ezVariantType::Uuid : pProperty->GetSpecificType()->GetVariantType();

  switch (pProperty->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      if (isValueType)
      {
        if (pProperty->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
          return true;
        return pAttrib->GetValue().CanConvertTo(type);
      }
      else if (pProperty->GetSpecificType()->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
      {
        return pAttrib->GetValue().CanConvertTo(ezVariantType::Int64);
      }
      else // Class
      {
        return false;
      }
    }
    break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      if (!isValueType)
        return true;

      if (!pAttrib->GetValue().IsA<ezVariantArray>())
        return false;

      const auto& defaultArray = pAttrib->GetValue().Get<ezVariantArray>();
      for (ezUInt32 i = 0; i < defaultArray.GetCount(); ++i)
      {
        const ezVariant& defaultSubValue = defaultArray[i];
        if (pProperty->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
          continue;
        if (!defaultSubValue.CanConvertTo(type))
          return false;
      }
      return true;
    }
    break;
    case ezPropertyCategory::Map:
    {
      if (isValueType)
        return true;

      if (pAttrib->GetValue().IsA<ezVariantDictionary>())
        return false;

      const auto& defaultDict = pAttrib->GetValue().Get<ezVariantDictionary>();
      for (auto it = defaultDict.GetIterator(); it.IsValid(); ++it)
      {
        const ezVariant& defaultSubValue = it.Value();
        if (pProperty->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
          continue;
        if (!defaultSubValue.CanConvertTo(type))
          return false;
      }
      return true;
    }
    break;
    default:
      break;
  }
  return true;
}

ezTestAppRun ezEditorTestMisc::DefaultValuesTest()
{
  ezRTTI::ForEachType([&](const ezRTTI* pRtti)
    {
      ezArrayPtr<const ezAbstractProperty* const> props = pRtti->GetProperties();
      for (const ezAbstractProperty* pProperty : props)
      {
        const ezDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<ezDefaultValueAttribute>();
        if (!pAttrib)
          return;

        if (!CheckDefaultValue(pProperty, pAttrib))
        {
          ezLog::Error("Invalid default value property! Type: {}, Property: {}, PropertyType: {}, DefaultValueType: {}", pRtti->GetTypeName(), pProperty->GetPropertyName(), pProperty->GetSpecificType()->GetTypeName(), pAttrib->GetValue().GetReflectedType()->GetTypeName());
        }
      } });

  return ezTestAppRun::Quit;
}

ezTestAppRun ezEditorTestMisc::AssetBrowerModelTest()
{
  ezQtAssetBrowserWidget* pDialog = new ezQtAssetBrowserWidget(QApplication::activeWindow());
  pDialog->SetMode(ezQtAssetBrowserWidget::Mode::Browser);
  pDialog->show();
  ProcessEvents();
  ezQtAssetBrowserFilter* pFilter = pDialog->GetAssetBrowserFilter();
  ezQtAssetBrowserModel* pModel = pDialog->GetAssetBrowserModel();
  pFilter->SetPathFilter("EditorTest/Meshes");
  pFilter->SetShowNonImportableFiles(false);
  pFilter->SetShowFiles(true);
  ProcessEvents();

  // Importable asset found:
  {
    EZ_TEST_INT(pModel->rowCount(), 1);
    QModelIndex index = pModel->index(0, 0);
    QString relativePath = pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString();
    EZ_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/Cube.obj");
    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)pModel->data(index, ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    EZ_TEST_INT(itemType.GetValue(), ezAssetBrowserItemFlags::File);
    const bool bImportable = index.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();
    EZ_TEST_BOOL(bImportable);
  }

  // Import (manually) into mesh.ezMeshAsset
  ezUuid guid;
  {
    ezStringBuilder sName = m_sProjectPath;
    sName.AppendPath("Meshes", "mesh.ezMeshAsset");
    ezAssetDocument* pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::Default));
    ezDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    pAcc->StartTransaction("Edit Mesh");
    EZ_TEST_BOOL(pAcc->SetValueByName(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();
    EZ_TEST_STATUS(pDoc->SaveDocument());
    guid = pDoc->GetGuid();
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }

  // Wait for changes
  for (ezUInt32 i = 0; i < 10; ++i)
  {
    ProcessEvents();
    if (pModel->rowCount() == 2)
    {
      QModelIndex index2 = pModel->index(1, 0);
      ezUuid guid2 = pModel->data(index2, ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
      if (guid2 == guid)
        break;
    }
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
  }
  // Check new asset
  if (EZ_TEST_INT(pModel->rowCount(), 2))
  {
    QModelIndex index2 = pModel->index(1, 0);
    ezUuid guid2 = index2.data(ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
    EZ_TEST_BOOL(guid2 == guid);
    QString relativePath = index2.data(ezQtAssetBrowserModel::UserRoles::RelativePath).toString();
    EZ_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/mesh.ezMeshAsset");
    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index2.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    EZ_TEST_BOOL(itemType == (ezAssetBrowserItemFlags::File | ezAssetBrowserItemFlags::Asset));
    const bool bImportable = index2.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();
    EZ_TEST_BOOL(!bImportable);
  }

  // Rename Cobe.obj to zzz.obj, moving it to the back of the list and making the asset invalid
  {
    QModelIndex index = pModel->index(0, 0);
    QString absolutePath = index.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

    EZ_TEST_BOOL(pModel->setData(index, QString("zzz"), Qt::EditRole));
  }

  // Wait for changes
  for (ezUInt32 i = 0; i < 10; ++i)
  {
    ProcessEvents();
    if (pModel->rowCount() == 2)
    {
      QModelIndex index2 = pModel->index(1, 0);
      QString sName = index2.data(Qt::EditRole).toString();

      if (sName.toUtf8().data() == "zzz"_ezsv)
        break;
    }
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
  }

  // Check renamed file
  {
    QModelIndex index = pModel->index(1, 0);
    QString relativePath = pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString();

    EZ_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/zzz.obj");
    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)pModel->data(index, ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    EZ_TEST_INT(itemType.GetValue(), ezAssetBrowserItemFlags::File);
    const bool bImportable = index.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();
    EZ_TEST_BOOL(bImportable);
  }

  // Check asset
  {
    QModelIndex index = pModel->index(0, 0);
    ezUuid guid2 = index.data(ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
    EZ_TEST_BOOL(guid2 == guid);
    QString relativePath = index.data(ezQtAssetBrowserModel::UserRoles::RelativePath).toString();
    EZ_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/mesh.ezMeshAsset");
    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    EZ_TEST_BOOL(itemType == (ezAssetBrowserItemFlags::File | ezAssetBrowserItemFlags::Asset));
    const bool bImportable = index.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();
    EZ_TEST_BOOL(!bImportable);
  }

  // Wait for transform state of asset to change
  ezAssetInfo::TransformState state = ezAssetInfo::Unknown;
  for (ezUInt32 i = 0; i < 10; ++i)
  {
    ProcessEvents();

    QModelIndex index = pModel->index(0, 0);
    state = (ezAssetInfo::TransformState)index.data(ezQtAssetBrowserModel::UserRoles::TransformState).toInt();
    if (state == ezAssetInfo::TransformState::MissingTransformDependency)
      break;

    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
  }
  EZ_TEST_BOOL(state == ezAssetInfo::TransformState::MissingTransformDependency);
  pDialog->hide();
  delete pDialog;
  ProcessEvents();
  return ezTestAppRun::Quit;
}
