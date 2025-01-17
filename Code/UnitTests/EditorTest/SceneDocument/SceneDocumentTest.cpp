#include <EditorTest/EditorTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/SceneDocument/SceneDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QMimeData>

static ezEditorSceneDocumentTest s_EditorSceneDocumentTest;

const char* ezEditorSceneDocumentTest::GetTestName() const
{
  return "Scene Document Tests";
}

void ezEditorSceneDocumentTest::SetupSubTests()
{
  AddSubTest("Layer Operations", SubTests::ST_LayerOperations);
  AddSubTest("Prefab Operations", SubTests::ST_PrefabOperations);
  AddSubTest("Component Operations", SubTests::ST_ComponentOperations);
  AddSubTest("Object Property Path", SubTests::ST_ObjectPropertyPath);
}

ezResult ezEditorSceneDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::CreateAndLoadProject("SceneTestProject").Failed())
    return EZ_FAILURE;

  if (ezStatus res = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::None); res.Failed())
  {
    ezLog::Error("Asset transform failed: {}", res.m_sMessage);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezEditorSceneDocumentTest::DeInitializeTest()
{
  m_pDoc = nullptr;
  m_pLayer = nullptr;
  m_SceneGuid = ezUuid::MakeInvalid();
  m_LayerGuid = ezUuid::MakeInvalid();

  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorSceneDocumentTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_LayerOperations:
      LayerOperations();
      break;
    case SubTests::ST_PrefabOperations:
      PrefabOperations();
      break;
    case SubTests::ST_ComponentOperations:
      ComponentOperations();
      break;
    case SubTests::ST_ObjectPropertyPath:
      ObjectPropertyPath();
      break;
  }
  return ezTestAppRun::Quit;
}

ezResult ezEditorSceneDocumentTest::CreateSimpleScene(const char* szSceneName)
{
  ezStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath(szSceneName);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    m_pDoc = static_cast<ezScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    if (!EZ_TEST_BOOL(m_pDoc != nullptr))
      return EZ_FAILURE;

    EZ_ANALYSIS_ASSUME(m_pDoc != nullptr);
    m_SceneGuid = m_pDoc->GetGuid();
    ProcessEvents();
    EZ_TEST_STATUS(m_pDoc->CreateLayer("Layer1", m_LayerGuid));
    m_pLayer = ezDynamicCast<ezLayerDocument*>(m_pDoc->GetLayerDocument(m_LayerGuid));
    if (!EZ_TEST_BOOL(m_pLayer != nullptr))
      return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

void ezEditorSceneDocumentTest::CloseSimpleScene()
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Close Document")
  {
    bool bSaved = false;
    ezTaskGroupID id = m_pDoc->SaveDocumentAsync(
      [&bSaved](ezDocument* pDoc, ezStatus res)
      {
        bSaved = true;
      },
      true);

    m_pDoc->GetDocumentManager()->CloseDocument(m_pDoc);
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id));
    EZ_TEST_BOOL(bSaved);
    m_pDoc = nullptr;
    m_pLayer = nullptr;
    m_SceneGuid = ezUuid::MakeInvalid();
    m_LayerGuid = ezUuid::MakeInvalid();
  }
}

void ezEditorSceneDocumentTest::LayerOperations()
{
  ezStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath("LayerOperations.ezScene");

  ezScene2Document* pDoc = nullptr;
  ezEventSubscriptionID layerEventsID = 0;
  ezHybridArray<ezScene2LayerEvent, 2> expectedEvents;
  ezUuid sceneGuid;
  ezUuid layer1Guid;
  ezLayerDocument* pLayer1 = nullptr;

  auto TestLayerEvents = [&expectedEvents](const ezScene2LayerEvent& e)
  {
    if (EZ_TEST_BOOL(!expectedEvents.IsEmpty()))
    {
      // If we pass in an invalid guid it's considered fine as we might not know the ID, e.g. when creating a layer.
      EZ_TEST_BOOL(!expectedEvents[0].m_layerGuid.IsValid() || expectedEvents[0].m_layerGuid == e.m_layerGuid);
      EZ_TEST_BOOL(expectedEvents[0].m_Type == e.m_Type);
      expectedEvents.RemoveAtAndCopy(0);
    }
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    pDoc = static_cast<ezScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    if (!EZ_TEST_BOOL(pDoc != nullptr))
      return;

    EZ_ANALYSIS_ASSUME(pDoc != nullptr);
    sceneGuid = pDoc->GetGuid();
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Layer")
  {
    EZ_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    EZ_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    EZ_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerAdded, ezUuid()});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerLoaded, ezUuid()});
    EZ_TEST_STATUS(pDoc->CreateLayer("Layer1", layer1Guid));

    expectedEvents.PushBack({ezScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    EZ_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    EZ_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    EZ_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));
    pLayer1 = ezDynamicCast<ezLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    EZ_TEST_BOOL(pLayer1 != nullptr);

    ezHybridArray<ezSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    EZ_TEST_INT(layers.GetCount(), 2);
    EZ_TEST_BOOL(layers.Contains(pLayer1));
    EZ_TEST_BOOL(layers.Contains(pDoc));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Undo/Redo Layer Creation")
  {
    // Undo / redo
    expectedEvents.PushBack({ezScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    EZ_TEST_STATUS(pDoc->SetActiveLayer(sceneGuid));
    // Initial scene setup exists in the scene undo stack
    const ezUInt32 uiInitialUndoStackSize = pDoc->GetCommandHistory()->GetUndoStackSize();
    EZ_TEST_BOOL(uiInitialUndoStackSize >= 1);
    EZ_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), uiInitialUndoStackSize);
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    EZ_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    EZ_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));

    pLayer1 = ezDynamicCast<ezLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    EZ_TEST_BOOL(pLayer1 != nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Save and Close Document")
  {
    bool bSaved = false;
    ezTaskGroupID id = pDoc->SaveDocumentAsync(
      [&bSaved](ezDocument* pDoc, ezStatus res)
      {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id));
    EZ_TEST_BOOL(bSaved);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reload Document")
  {
    pDoc = static_cast<ezScene2Document*>(m_pApplication->m_pEditorApp->OpenDocument(sName, ezDocumentFlags::RequestWindow));
    if (!EZ_TEST_BOOL(pDoc != nullptr))
      return;

    EZ_ANALYSIS_ASSUME(pDoc != nullptr);
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();

    EZ_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    EZ_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    EZ_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    pLayer1 = ezDynamicCast<ezLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    ezHybridArray<ezSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    EZ_TEST_INT(layers.GetCount(), 2);
    EZ_TEST_BOOL(layers.Contains(pLayer1));
    EZ_TEST_BOOL(layers.Contains(pDoc));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Toggle Layer Visibility")
  {
    EZ_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    expectedEvents.PushBack({ezScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    EZ_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    EZ_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    EZ_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));
    EZ_TEST_BOOL(!pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerVisible, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, true));
    EZ_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Toggle Layer Loaded")
  {
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));

    expectedEvents.PushBack({ezScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, false));

    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, true));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete Layer")
  {
    expectedEvents.PushBack({ezScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    EZ_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));

    expectedEvents.PushBack({ezScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    EZ_TEST_STATUS(pDoc->DeleteLayer(layer1Guid));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Undo/Redo Layer Deletion")
  {
    EZ_TEST_INT(pDoc->GetCommandHistory()->GetUndoStackSize(), 1);
    EZ_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), 1);
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    EZ_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    EZ_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    EZ_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({ezScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    EZ_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Close Document")
  {
    bool bSaved = false;
    ezTaskGroupID id = pDoc->SaveDocumentAsync(
      [&bSaved](ezDocument* pDoc, ezStatus res)
      {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id));
    EZ_TEST_BOOL(bSaved);
  }
}


void ezEditorSceneDocumentTest::PrefabOperations()
{
  if (CreateSimpleScene("PrefabOperations.ezScene").Failed())
    return;

  const ezDocumentObject* pPrefab1 = nullptr;
  const ezDocumentObject* pPrefab2 = nullptr;
  auto pAccessor = m_pDoc->GetObjectAccessor();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Drag&Drop Prefabs")
  {
    const char* szSpherePrefab = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }";
    pPrefab1 = DropAsset(m_pDoc, szSpherePrefab);
    EZ_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    EZ_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
    pPrefab2 = DropAsset(m_pDoc, szSpherePrefab, true);
    EZ_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    EZ_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Nodes and check default state")
  {
    const char* szSphereMesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }";
    const ezDocumentObject* pSphere1 = DropAsset(m_pDoc, szSphereMesh);
    const ezDocumentObject* pSphere2 = DropAsset(m_pDoc, szSphereMesh);

    pAccessor->StartTransaction("Modify objects");
    EZ_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "Name", "Sphere1"));
    EZ_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "LocalPosition", ezVec3(1.0f, 0.0f, 0.0f)));
    EZ_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "LocalRotation", ezQuat(1.0f, 0.0f, 0.0f, 0.0f)));
    EZ_TEST_STATUS(pAccessor->SetValueByName(pSphere1, "LocalScaling", ezVec3(1.0f, 2.0f, 3.0f)));
    EZ_TEST_STATUS(pAccessor->InsertValueByName(pSphere1, "Tags", "SkyLight", -1));
    const ezDocumentObject* pMeshComponent = pAccessor->GetObject(pAccessor->GetByName<ezVariantArray>(pSphere1, "Components")[0].Get<ezUuid>());
    EZ_TEST_STATUS(pAccessor->InsertValueByName(pMeshComponent, "Materials", "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }", 0));

    ezUuid pSphereRef;
    EZ_TEST_STATUS(pAccessor->AddObjectByName(pSphere1, "Components", -1, ezGetStaticRTTI<ezSphereReflectionProbeComponent>(), pSphereRef));

    pAccessor->FinishTransaction();

    {
      // Check that modifications above changed properties from their default state.
      ezHybridArray<ezPropertySelection, 1> selection;
      selection.PushBack({pSphere1, ezVariant()});
      ezDefaultObjectState defaultState(pAccessor, selection);
      EZ_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");

      EZ_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("Components"));

      // Does default state match that of pSphere2 which is unmodified?
      auto MatchesDefaultValue = [&](ezDefaultObjectState& ref_defaultState, const char* szProperty)
      {
        ezVariant defaultValue = ref_defaultState.GetDefaultValue(szProperty);
        ezVariant sphere2value;
        EZ_TEST_STATUS(pAccessor->GetValueByName(pSphere2, szProperty, sphere2value));
        EZ_TEST_BOOL(defaultValue == sphere2value);
      };

      MatchesDefaultValue(defaultState, "Name");
      MatchesDefaultValue(defaultState, "LocalPosition");
      MatchesDefaultValue(defaultState, "LocalRotation");
      MatchesDefaultValue(defaultState, "LocalScaling");
      MatchesDefaultValue(defaultState, "Tags");
    }

    {
      // pSphere2 should be unmodified except for the component array.
      ezHybridArray<ezPropertySelection, 1> selection;
      selection.PushBack({pSphere2, ezVariant()});
      ezDefaultObjectState defaultState(pAccessor, selection);
      EZ_TEST_BOOL(defaultState.IsDefaultValue("Name"));
      EZ_TEST_BOOL(defaultState.IsDefaultValue("LocalPosition"));
      EZ_TEST_BOOL(defaultState.IsDefaultValue("LocalRotation"));
      EZ_TEST_BOOL(defaultState.IsDefaultValue("LocalScaling"));
      EZ_TEST_BOOL(defaultState.IsDefaultValue("Tags"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Multi-selection should not be default if one in the selection is not.
      ezHybridArray<ezPropertySelection, 1> selection;
      selection.PushBack({pSphere1, ezVariant()});
      selection.PushBack({pSphere2, ezVariant()});
      ezDefaultObjectState defaultState(pAccessor, selection);
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      EZ_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Default state object array
      ezHybridArray<ezPropertySelection, 1> selection;
      selection.PushBack({pSphere1, ezVariant()});
      ezDefaultContainerState defaultState(pAccessor, selection, "Components");
      EZ_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      EZ_TEST_BOOL(defaultState.GetDefaultContainer() == ezVariantArray());
      EZ_TEST_BOOL(defaultState.GetDefaultElement(0) == ezUuid());
      EZ_TEST_BOOL(!defaultState.IsDefaultContainer());
      // We currently do not supporting reverting an index of a non-value type container. Thus, they are always the default state.
      EZ_TEST_BOOL(defaultState.IsDefaultElement(0));
      EZ_TEST_BOOL(defaultState.IsDefaultElement(1));
    }

    {
      // Default state value array
      ezHybridArray<ezPropertySelection, 1> selection;
      selection.PushBack({pMeshComponent, ezVariant()});
      ezDefaultContainerState defaultState(pAccessor, selection, "Materials");
      EZ_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      EZ_TEST_BOOL(defaultState.GetDefaultContainer() == ezVariantArray());
      EZ_TEST_BOOL(defaultState.GetDefaultElement(0) == "");
      EZ_TEST_BOOL(!defaultState.IsDefaultContainer());
      EZ_TEST_BOOL(!defaultState.IsDefaultElement(0));

      ezDefaultObjectState defaultObjectState(pAccessor, selection);
      EZ_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      EZ_TEST_BOOL(defaultObjectState.GetDefaultValue("Materials") == ezVariantArray());
      EZ_TEST_BOOL(!defaultObjectState.IsDefaultValue("Materials"));
    }

    ezDeque<const ezDocumentObject*> selection;
    selection.PushBack(pSphere1);
    selection.PushBack(pSphere2);
    m_pDoc->GetSelectionManager()->SetSelection(selection);
  }

  ezUuid prefabGuid;
  const ezDocumentObject* pPrefab3 = nullptr;
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Prefab from Selection")
  {
    // ProcessEvents(999999999);

    ezStringBuilder sPrefabName;
    sPrefabName = m_sProjectPath;
    sPrefabName.AppendPath("Spheres.ezPrefab");
    EZ_TEST_BOOL(m_pDoc->CreatePrefabDocumentFromSelection(sPrefabName, ezGetStaticRTTI<ezGameObject>(), {}, {}, [](ezAbstractObjectGraph& graph, ezDynamicArray<ezAbstractObjectNode*>&) { /* do nothing */ }).Succeeded());
    m_pDoc->ScheduleSendObjectSelection();
    pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
    EZ_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    EZ_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuid));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move Prefabs to Layer")
  {
    EZ_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    EZ_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    EZ_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());

    // Copy & paste should retain the order in the tree view, not the selection array so we push the elements in a random order here.
    ezDeque<const ezDocumentObject*> assets;
    assets.PushBack(pPrefab1);
    assets.PushBack(pPrefab2);
    assets.PushBack(pPrefab3);
    ezDeque<const ezDocumentObject*> newObjects;

    MoveObjectsToLayer(m_pDoc, assets, m_LayerGuid, newObjects);

    EZ_TEST_BOOL(m_pDoc->GetActiveLayer() == m_SceneGuid);
    EZ_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab1->GetGuid()) == nullptr);
    EZ_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab2->GetGuid()) == nullptr);
    EZ_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab3->GetGuid()) == nullptr);

    EZ_TEST_INT(newObjects.GetCount(), assets.GetCount());
    pPrefab1 = newObjects[0];
    pPrefab2 = newObjects[1];
    pPrefab3 = newObjects[2];

    EZ_TEST_STATUS(m_pDoc->SetActiveLayer(m_LayerGuid));

    EZ_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    EZ_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    EZ_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pLayer->GetObjectManager());

    EZ_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    EZ_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
    EZ_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    EZ_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
    EZ_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    ezUuid prefabGuidOut;
    EZ_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuidOut));
    EZ_TEST_BOOL(prefabGuid == prefabGuidOut);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Change Prefab Type")
  {
    ezVariant oldIndex = pPrefab1->GetPropertyIndex();
    ezHybridArray<const ezDocumentObject*, 8> selection;
    {
      selection.PushBack(pPrefab1);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab1 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      EZ_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
      EZ_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
      EZ_TEST_BOOL(oldIndex == pPrefab1->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab2->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab2);
      m_pDoc->ConvertToEnginePrefab(selection);
      pPrefab2 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      EZ_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
      EZ_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
      EZ_TEST_BOOL(oldIndex == pPrefab2->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab3->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab3);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      EZ_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid()));
      ezUuid prefabGuidOut;
      EZ_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      EZ_TEST_BOOL(prefabGuid == prefabGuidOut);
      EZ_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
    }
  }

  auto IsObjectDefault = [&](const ezDocumentObject* pChild)
  {
    ezHybridArray<ezPropertySelection, 1> selection;
    selection.PushBack({pChild, ezVariant()});
    ezDefaultObjectState defaultState(pAccessor, selection);
    // The root node of the prefab is not actually part of the prefab in the sense that it is just the container and does not actually exist in the prefab itself.
    const char* szExpectedProvider = pChild == pPrefab3 ? "Attribute" : "Prefab";
    EZ_TEST_STRING(defaultState.GetStateProviderName(), szExpectedProvider);

    ezHybridArray<const ezAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Hidden | ezPropertyFlags::ReadOnly))
        continue;

      EZ_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Modify Editor Prefab")
  {
    ezVariant oldIndex = pPrefab3->GetPropertyIndex();
    const ezAbstractProperty* pProp = pPrefab3->GetType()->FindPropertyByName("Children");
    const ezAbstractProperty* pCompProp = pPrefab3->GetType()->FindPropertyByName("Components");

    // ProcessEvents(999999999);

    CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);

    // ProcessEvents(999999999);
    {
      m_pDoc->UpdatePrefabs();
      // Update prefabs replaces object instances with new ones with the same IDs so the old ones are in the undo history now.
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Remove part of the prefab
      ezHybridArray<ezVariant, 16> values;
      EZ_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      EZ_TEST_INT(values.GetCount(), 2);
      const ezDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<ezUuid>());
      const ezDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<ezUuid>());
      pAccessor->StartTransaction("Delete child0");
      pAccessor->RemoveObject(pChild1).AssertSuccess();
      pAccessor->FinishTransaction();
      EZ_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 1);
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert prefab
      ezHybridArray<const ezDocumentObject*, 2> selection;
      selection.PushBack(pPrefab3);
      m_pDoc->RevertPrefabs(selection);

      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      ezUuid prefabGuidOut;
      EZ_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      EZ_TEST_BOOL(prefabGuid == prefabGuidOut);
      EZ_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
      EZ_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
    }

    {
      // Modify the prefab
      ezHybridArray<ezVariant, 16> values;
      EZ_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      EZ_TEST_INT(values.GetCount(), 2);
      const ezDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<ezUuid>());
      const ezDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<ezUuid>());

      pAccessor->StartTransaction("Modify Prefab");
      ezUuid compGuid;
      EZ_TEST_STATUS(pAccessor->AddObjectByName(pChild0, "Components", -1, ezRTTI::FindTypeByName("ezBeamComponent"), compGuid));
      const ezDocumentObject* pComp = pAccessor->GetObject(compGuid);

      const ezDocumentObject* pChild1Comp = pAccessor->GetChildObjectByName(pChild1, "Components", 0);
      EZ_TEST_STATUS(pAccessor->RemoveObject(pChild1Comp));
      pAccessor->FinishTransaction();

      EZ_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
      EZ_TEST_INT(pAccessor->GetCount(pChild0, pCompProp), 3);
      EZ_TEST_INT(pAccessor->GetCount(pChild1, pCompProp), 0);

      // Check default states
      {
        ezHybridArray<ezPropertySelection, 1> selection;
        selection.PushBack({pChild0, ezVariant()});
        ezDefaultContainerState defaultObjectState(pAccessor, selection, "Components");
        EZ_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        EZ_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        ezHybridArray<ezPropertySelection, 1> selection;
        selection.PushBack({pChild1, ezVariant()});
        ezDefaultContainerState defaultObjectState(pAccessor, selection, "Components");
        EZ_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        EZ_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        ezHybridArray<ezPropertySelection, 1> selection;
        selection.PushBack({pComp, ezVariant()});
        ezDefaultObjectState defaultObjectState(pAccessor, selection);
        EZ_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      }
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert via default state
      const ezDocumentObject* pChild1 = pAccessor->GetChildObjectByName(pPrefab3, "Children", 0);
      const ezDocumentObject* pChild2 = pAccessor->GetChildObjectByName(pPrefab3, "Children", 1);
      {
        ezHybridArray<ezPropertySelection, 1> selection;
        selection.PushBack({pChild1, ezVariant()});
        selection.PushBack({pChild2, ezVariant()});
        ezDefaultContainerState defaultState(pAccessor, selection, "Components");

        pAccessor->StartTransaction("Revert children");
        defaultState.RevertContainer().AssertSuccess();
        pAccessor->FinishTransaction();
      }
    }

    {
      // Verify prefab was reverted
      ezHybridArray<ezVariant, 16> values;
      EZ_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      EZ_TEST_INT(values.GetCount(), 2);
      const ezDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<ezUuid>());
      const ezDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<ezUuid>());

      values.Clear();
      EZ_TEST_STATUS(pAccessor->GetValuesByName(pChild0, "Components", values));
      EZ_TEST_INT(values.GetCount(), 2);

      EZ_TEST_STATUS(pAccessor->GetValuesByName(pChild1, "Components", values));
      EZ_TEST_INT(values.GetCount(), 1);

      CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);
    }
  }

  ProcessEvents(10);
  CloseSimpleScene();
}

void ezEditorSceneDocumentTest::ComponentOperations()
{
  if (CreateSimpleScene("ComponentOperations.ezScene").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  const ezDocumentObject* pRoot = CreateGameObject(m_pDoc);

  ezDeque<const ezDocumentObject*> selection;
  selection.PushBack(pRoot);
  m_pDoc->GetSelectionManager()->SetSelection(selection);

  auto CreateComponent = [&](const ezRTTI* pType, const ezDocumentObject* pParent) -> const ezDocumentObject*
  {
    ezUuid compGuid;
    EZ_TEST_STATUS(pAccessor->AddObjectByName(pParent, "Components", -1, pType, compGuid));
    return pAccessor->GetObject(compGuid);
  };

  auto IsObjectDefault = [&](const ezDocumentObject* pChild)
  {
    ezHybridArray<ezPropertySelection, 1> selection;
    selection.PushBack({pChild, ezVariant()});
    ezDefaultObjectState defaultState(pAccessor, selection);

    ezHybridArray<const ezAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Hidden | ezPropertyFlags::ReadOnly))
        continue;

      ezVariant defaultValue = defaultState.GetDefaultValue(pProp);
      EZ_TEST_BOOL(ezDefaultStateProvider::DoesVariantMatchProperty(defaultValue, pProp));
      ezVariant currentValue;
      EZ_TEST_STATUS(pAccessor->GetValue(pChild, pProp, currentValue));
      EZ_TEST_BOOL(ezDefaultStateProvider::DoesVariantMatchProperty(currentValue, pProp));
      EZ_TEST_BOOL(defaultValue == currentValue);
      EZ_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  ezDynamicArray<const ezRTTI*> componentTypes;
  ezRTTI::ForEachDerivedType<ezComponent>([&](const ezRTTI* pRtti)
    { componentTypes.PushBack(pRtti); });

  ezSet<const ezRTTI*> blacklist;
  // The scene already has one and the code asserts otherwise. There needs to be a general way of preventing two settings components from existing at the same time.
  blacklist.Insert(ezRTTI::FindTypeByName("ezSkyLightComponent"));

  pAccessor->StartTransaction("Modify objects");

  for (auto pType : componentTypes)
  {
    if (pType->GetTypeFlags().IsSet(ezTypeFlags::Abstract) || blacklist.Contains(pType))
      continue;

    auto pComp = CreateComponent(pType, pRoot);

    CheckHierarchy(pAccessor, pComp, IsObjectDefault);
  }

  pAccessor->FinishTransaction();
  ProcessEvents(10);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Re-open document")
  {
    ezUuid layerGuid = m_LayerGuid;
    CloseSimpleScene();
    ProcessEvents(10);

    m_pDoc = ezDynamicCast<ezScene2Document*>(OpenDocument("ComponentOperations.ezScene"));
    EZ_TEST_BOOL(m_pDoc != nullptr);
    m_SceneGuid = m_pDoc->GetGuid();

    ezHybridArray<ezUuid, 2> layers;
    m_pDoc->GetAllLayers(layers);

    EZ_TEST_BOOL(layers.Contains(layerGuid));
    m_LayerGuid = layerGuid;

    EZ_TEST_BOOL(m_pDoc->SetLayerLoaded(m_LayerGuid, true).Succeeded());
    m_pLayer = ezDynamicCast<ezLayerDocument*>(m_pDoc->GetLayerDocument(m_LayerGuid));
    EZ_TEST_BOOL(m_pLayer != nullptr);
  }

  CloseSimpleScene();
}


void ezEditorSceneDocumentTest::ObjectPropertyPath()
{
  if (CreateSimpleScene("ObjectPropertyPath.ezScene").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  auto CreateComponent = [&](const ezDocumentObject* pParent) -> const ezDocumentObject*
  {
    pAccessor->StartTransaction("AddComponent"_ezsv);
    ezUuid compGuid;
    EZ_TEST_STATUS(pAccessor->AddObjectByName(pParent, "Components", -1, ezRTTI::FindTypeByName("ezDecalComponent"), compGuid));
    const ezDocumentObject* pComp = pAccessor->GetObject(compGuid);
    EZ_TEST_STATUS(pAccessor->InsertValueByName(pComp, "Decals", "", 0));
    pAccessor->FinishTransaction();
    return pComp;
  };

  const ezDocumentObject* pRoot = CreateGameObject(m_pDoc, nullptr, "Root"_ezsv);
  const ezDocumentObject* pDummy = CreateGameObject(m_pDoc, pRoot, "Dummy"_ezsv);
  const ezDocumentObject* pC1 = CreateGameObject(m_pDoc, pDummy, "C"_ezsv);
  const ezDocumentObject* pComp1 = CreateComponent(pC1);

  const ezDocumentObject* pA = CreateGameObject(m_pDoc, pRoot, "A"_ezsv);
  const ezDocumentObject* pB = CreateGameObject(m_pDoc, pA, "B"_ezsv);
  const ezDocumentObject* pC2 = CreateGameObject(m_pDoc, pB, "C"_ezsv);
  const ezDocumentObject* pComp2 = CreateComponent(pC2);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GameObject property")
  {
    ezObjectPropertyPathContext context{pRoot, pAccessor, "Children"};
    ezPropertyReference propertyRef{pC2->GetGuid(), pC2->GetType()->FindPropertyByName("Active"_ezsv)};

    ezStringBuilder sObjectSearchSequence;
    ezStringBuilder sComponentType;
    ezStringBuilder sPropertyPath;
    EZ_TEST_STATUS(ezObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath));
    EZ_TEST_STRING(sObjectSearchSequence, "A/B/C");
    EZ_TEST_BOOL(sComponentType.IsEmpty());
    EZ_TEST_STRING(sPropertyPath, "Active");

    ezHybridArray<ezPropertyReference, 2> properties;
    EZ_TEST_BOOL(ezObjectPropertyPath::ResolvePath(context, properties, "A/B/D", "", sPropertyPath).Failed());                                    // Path does not exist.
    EZ_TEST_BOOL(ezObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, "ezPointLightComponent", sPropertyPath).Failed()); // Component does not exist.
    EZ_TEST_BOOL(ezObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, "", "Bla").Failed());                              // Property does not exist.
    EZ_TEST_STATUS(ezObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, "", sPropertyPath));
    EZ_TEST_INT(properties.GetCount(), 1);
    EZ_TEST_BOOL(properties[0] == propertyRef);

    properties.Clear();
    EZ_TEST_STATUS(ezObjectPropertyPath::ResolvePath(context, properties, "C", "", sPropertyPath)); // ambiguous target
    EZ_TEST_INT(properties.GetCount(), 2);
    ezPropertyReference propertyRef2{pC1->GetGuid(), pC2->GetType()->FindPropertyByName("Active"_ezsv)};
    EZ_TEST_BOOL(properties[0] == propertyRef2);
    EZ_TEST_BOOL(properties[1] == propertyRef);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component member property")
  {
    ezObjectPropertyPathContext context{pRoot, pAccessor, "Children"};
    ezPropertyReference propertyRef{pComp2->GetGuid(), pComp2->GetType()->FindPropertyByName("Color"_ezsv)};

    ezStringBuilder sObjectSearchSequence;
    ezStringBuilder sComponentType;
    ezStringBuilder sPropertyPath;
    EZ_TEST_STATUS(ezObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath));
    EZ_TEST_STRING(sObjectSearchSequence, "A/B/C");
    EZ_TEST_STRING(sComponentType, "ezDecalComponent");
    EZ_TEST_STRING(sPropertyPath, "Color");

    ezHybridArray<ezPropertyReference, 2> properties;
    EZ_TEST_BOOL(ezObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, "Bla").Failed()); // Property does not exist.
    EZ_TEST_STATUS(ezObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, sPropertyPath));
    EZ_TEST_INT(properties.GetCount(), 1);
    EZ_TEST_BOOL(properties[0] == propertyRef);

    properties.Clear();
    EZ_TEST_STATUS(ezObjectPropertyPath::ResolvePath(context, properties, "C", sComponentType, sPropertyPath)); // ambiguous target
    EZ_TEST_INT(properties.GetCount(), 2);
    ezPropertyReference propertyRef2{pComp1->GetGuid(), pComp1->GetType()->FindPropertyByName("Color"_ezsv)};
    EZ_TEST_BOOL(properties[0] == propertyRef2);
    EZ_TEST_BOOL(properties[1] == propertyRef);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component array property")
  {
    ezObjectPropertyPathContext context{pRoot, pAccessor, "Children"};
    ezPropertyReference propertyRef{pComp2->GetGuid(), pComp2->GetType()->FindPropertyByName("Decals"_ezsv), 0};

    ezStringBuilder sObjectSearchSequence;
    ezStringBuilder sComponentType;
    ezStringBuilder sPropertyPath;
    EZ_TEST_STATUS(ezObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath));
    EZ_TEST_STRING(sObjectSearchSequence, "A/B/C");
    EZ_TEST_STRING(sComponentType, "ezDecalComponent");
    EZ_TEST_STRING(sPropertyPath, "Decals[0]");

    ezHybridArray<ezPropertyReference, 2> properties;
    EZ_TEST_BOOL(ezObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, "Decals[1]").Failed()); // Index out of range.
    EZ_TEST_STATUS(ezObjectPropertyPath::ResolvePath(context, properties, sObjectSearchSequence, sComponentType, sPropertyPath));
    EZ_TEST_INT(properties.GetCount(), 1);
    EZ_TEST_BOOL(properties[0] == propertyRef);

    properties.Clear();
    EZ_TEST_STATUS(ezObjectPropertyPath::ResolvePath(context, properties, "C", sComponentType, sPropertyPath)); // ambiguous target
    EZ_TEST_INT(properties.GetCount(), 2);
    ezPropertyReference propertyRef2{pComp1->GetGuid(), pComp1->GetType()->FindPropertyByName("Decals"_ezsv), 0};
    EZ_TEST_BOOL(properties[0] == propertyRef2);
    EZ_TEST_BOOL(properties[1] == propertyRef);
  }

  CloseSimpleScene();
}

void ezEditorSceneDocumentTest::CheckHierarchy(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pRoot, ezDelegate<void(const ezDocumentObject* pChild)> functor)
{
  ezDeque<const ezDocumentObject*> objects;
  objects.PushBack(pRoot);
  while (!objects.IsEmpty())
  {
    const ezDocumentObject* pCurrent = objects[0];
    objects.PopFront();
    {
      functor(pCurrent);
    }

    for (auto pChild : pCurrent->GetChildren())
    {
      objects.PushBack(pChild);
    }
  }
}
