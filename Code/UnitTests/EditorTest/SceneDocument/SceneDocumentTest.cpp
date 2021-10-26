#include <EditorTest/EditorTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/SceneDocument/SceneDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <QMimeData>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezEditorSceneDocumentTest s_EditorSceneDocumentTest;

const char* ezEditorSceneDocumentTest::GetTestName() const
{
  return "Scene Document Tests";
}

void ezEditorSceneDocumentTest::SetupSubTests()
{
  AddSubTest("Layer Operations", SubTests::ST_LayerOperations);
  AddSubTest("Prefab Operations", SubTests::ST_PrefabOperations);
}

ezResult ezEditorSceneDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::CreateAndLoadProject("SceneTestProject").Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezEditorSceneDocumentTest::DeInitializeTest()
{
  m_pDoc = nullptr;
  m_pLayer = nullptr;
  m_sceneGuid.SetInvalid();
  m_layerGuid.SetInvalid();

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

    m_sceneGuid = m_pDoc->GetGuid();
    ProcessEvents();
    EZ_TEST_STATUS(m_pDoc->CreateLayer("Layer1", m_layerGuid));
    m_pLayer = ezDynamicCast<ezLayerDocument*>(m_pDoc->GetLayerDocument(m_layerGuid));
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
      [&bSaved](ezDocument* doc, ezStatus res) {
        bSaved = true;
      },
      true);

    m_pDoc->GetDocumentManager()->CloseDocument(m_pDoc);
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id));
    EZ_TEST_BOOL(bSaved);
    m_pDoc = nullptr;
    m_pLayer = nullptr;
    m_sceneGuid.SetInvalid();
    m_layerGuid.SetInvalid();
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

  auto TestLayerEvents = [&expectedEvents](const ezScene2LayerEvent& e) {
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
    EZ_TEST_INT(pDoc->GetCommandHistory()->GetUndoStackSize(), 2);
    EZ_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), 2);
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
      [&bSaved](ezDocument* doc, ezStatus res) {
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
      [&bSaved](ezDocument* doc, ezStatus res) {
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

  ezUuid prefabGuid;
  const ezDocumentObject* pPrefab3 = nullptr;
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Prefab from Nodes")
  {
    const char* szSphereMesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }";
    const ezDocumentObject* pSphere1 = DropAsset(m_pDoc, szSphereMesh);
    const ezDocumentObject* pSphere2 = DropAsset(m_pDoc, szSphereMesh);
    ezDeque<const ezDocumentObject*> selection;
    selection.PushBack(pSphere1);
    selection.PushBack(pSphere2);
    m_pDoc->GetSelectionManager()->SetSelection(selection);

    ezStringBuilder sPrefabName;
    sPrefabName = m_sProjectPath;
    sPrefabName.AppendPath("Spheres.ezPrefab");
    EZ_TEST_BOOL(m_pDoc->CreatePrefabDocumentFromSelection(sPrefabName, ezGetStaticRTTI<ezGameObject>()).Succeeded());
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
    assets.PushBack(pPrefab3);
    assets.PushBack(pPrefab1);
    assets.PushBack(pPrefab2);
    ezDeque<const ezDocumentObject*> newObjects;

    MoveObjectsToLayer(m_pDoc, assets, m_layerGuid, newObjects);

    EZ_TEST_BOOL(m_pDoc->GetActiveLayer() == m_sceneGuid);
    EZ_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab1->GetGuid()) == nullptr);
    EZ_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab2->GetGuid()) == nullptr);
    EZ_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab3->GetGuid()) == nullptr);

    EZ_TEST_INT(newObjects.GetCount(), assets.GetCount());
    pPrefab1 = newObjects[0];
    pPrefab2 = newObjects[1];
    pPrefab3 = newObjects[2];

    EZ_TEST_STATUS(m_pDoc->SetActiveLayer(m_layerGuid));

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
    ezDeque<const ezDocumentObject*> selection;
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Modify Editor Prefab")
  {
    ezVariant oldIndex = pPrefab3->GetPropertyIndex();
    const ezAbstractProperty* pProp = pPrefab3->GetType()->FindPropertyByName("Children");
    auto pAccessor = m_pDoc->GetObjectAccessor();

    {
      // Remove part of the prefab
      ezHybridArray<ezVariant, 16> values;
      EZ_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      EZ_TEST_INT(values.GetCount(), 2);
      const ezDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<ezUuid>());
      const ezDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<ezUuid>());
      pAccessor->StartTransaction("Delete child0");
      pAccessor->RemoveObject(pChild1);
      pAccessor->FinishTransaction();
      EZ_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 1);
    }

    {
      // Revert prefab
      ezDeque<const ezDocumentObject*> selection;
      selection.PushBack(pPrefab3);
      m_pDoc->RevertPrefabs(selection);

      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      ezUuid prefabGuidOut;
      EZ_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      EZ_TEST_BOOL(prefabGuid == prefabGuidOut);
      EZ_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
      EZ_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
    }
  }
  //ProcessEvents(999999999);
  //return;
}
