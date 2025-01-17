#include <EditorTest/EditorTestPCH.h>

#include "Misc.h"
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
}

ezResult ezEditorTestMisc::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return EZ_FAILURE;

  if (ezStatus res = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::None); res.Failed())
  {
    ezLog::Error("Asset transform failed: {}", res.m_sMessage);
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
  //  ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::TriggeredManually);

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
