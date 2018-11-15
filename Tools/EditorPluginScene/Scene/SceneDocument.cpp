#include <PCH.h>

#include <Commands/SceneCommands.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <RendererCore/Components/CameraComponent.h>
#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, 4, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSceneDocument::ezSceneDocument(const char* szDocumentPath, bool bIsPrefab)
    : ezGameObjectDocument(szDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager))
{
  m_bIsPrefab = bIsPrefab;
  m_GameMode = GameMode::Off;
  SetAddAmbientLight(bIsPrefab);

  m_GameModeData[GameMode::Off].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Off].m_bRenderShapeIcons = true;
  m_GameModeData[GameMode::Off].m_bRenderVisualizers = true;

  m_GameModeData[GameMode::Simulate].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Simulate].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Simulate].m_bRenderVisualizers = false;

  m_GameModeData[GameMode::Play].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Play].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Play].m_bRenderVisualizers = false;
}


void ezSceneDocument::InitializeAfterLoading()
{
  // (Local mirror only mirrors settings)
  m_ObjectMirror.SetFilterFunction([this](const ezDocumentObject* pObject, const char* szProperty) -> bool {
    return GetObjectManager()->IsUnderRootProperty("Settings", pObject, szProperty);
  });
  // (Remote IPC mirror only sends scene)
  m_Mirror.SetFilterFunction([this](const ezDocumentObject* pObject, const char* szProperty) -> bool {
    return GetObjectManager()->IsUnderRootProperty("Children", pObject, szProperty);
  });

  ezGameObjectDocument::InitializeAfterLoading();
  EnsureSettingsObjectExist();

  m_DocumentObjectMetaData.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezSceneDocument::DocumentObjectMetaDataEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));
  ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(
      ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

ezSceneDocument::~ezSceneDocument()
{
  m_DocumentObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(
      ezMakeDelegate(&ezSceneDocument::DocumentObjectMetaDataEventHandler, this));

  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ToolsProjectEventHandler, this));

  ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(
      ezMakeDelegate(&ezSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();
}


const char* ezSceneDocument::GetDocumentTypeDisplayString() const
{
  if (m_bIsPrefab)
    return "Prefab";

  return "Scene";
}

void ezSceneDocument::GroupSelection()
{
  const auto& sel = GetSelectionManager()->GetTopLevelSelection(ezGetStaticRTTI<ezGameObject>());
  if (sel.GetCount() <= 1)
    return;

  ezVec3 vCenter(0.0f);

  for (const auto& item : sel)
  {
    vCenter += GetGlobalTransform(item).m_vPosition;
  }

  vCenter /= sel.GetCount();
  // vCenter.SetZero();

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Group Selection");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
  cmdAdd.m_Index = -1;
  cmdAdd.m_sParentProperty = "Children";

  pHistory->AddCommand(cmdAdd);

  auto pGroupObject = GetObjectManager()->GetObject(cmdAdd.m_NewObjectGuid);
  SetGlobalTransform(pGroupObject, ezTransform(vCenter), TransformationChanges::Translation);

  ezMoveObjectCommand cmdMove;
  cmdMove.m_NewParent = cmdAdd.m_NewObjectGuid;
  cmdMove.m_Index = -1;
  cmdMove.m_sParentProperty = "Children";

  for (const auto& item : sel)
  {
    cmdMove.m_Object = item->GetGuid();
    pHistory->AddCommand(cmdMove);
  }

  pHistory->FinishTransaction();
}


void ezSceneDocument::DuplicateSpecial()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  ezQtDuplicateDlg dlg(nullptr);
  if (dlg.exec() == QDialog::Rejected)
    return;

  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  ezStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", ezConversionUtils::ToString(it.Key(), tmp1), ezConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1); // null terminate

  ezDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;
  cmd.m_uiNumberOfCopies = dlg.s_uiNumberOfCopies;
  cmd.m_vAccumulativeTranslation = dlg.s_vTranslationStep;
  cmd.m_vAccumulativeRotation = dlg.s_vRotationStep;
  cmd.m_vRandomRotation = dlg.s_vRandomRotation;
  cmd.m_vRandomTranslation = dlg.s_vRandomTranslation;
  cmd.m_bGroupDuplicates = dlg.s_bGroupCopies;
  cmd.m_iRevolveAxis = dlg.s_iRevolveAxis;
  cmd.m_fRevolveRadius = dlg.s_fRevolveRadius;
  cmd.m_RevolveStartAngle = ezAngle::Degree(dlg.s_iRevolveStartAngle);
  cmd.m_RevolveAngleStep = ezAngle::Degree(dlg.s_iRevolveAngleStep);

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}


void ezSceneDocument::DeltaTransform()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  ezQtDeltaTransformDlg dlg(nullptr, this);
  dlg.exec();
}

void ezSceneDocument::SnapObjectToCamera()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  const auto& camera = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  ezMat3 mRot;

  ezTransform transform;
  transform.m_vScale.Set(1.0f);
  transform.m_vPosition = camera.GetCenterPosition();
  mRot.SetColumn(0, camera.GetCenterDirForwards());
  mRot.SetColumn(1, camera.GetCenterDirRight());
  mRot.SetColumn(2, camera.GetCenterDirUp());
  transform.m_qRotation.SetFromMat3(mRot);

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Snap Object to Camera");
  {
    for (const ezDocumentObject* pObject : selection)
    {
      SetGlobalTransform(pObject, transform, TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }
  pHistory->FinishTransaction();
}


void ezSceneDocument::AttachToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();
  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr ||
      !ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
    return;

  ezMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";
  cmd.m_NewParent = ctxt.m_pLastPickingResult->m_PickedObject;
  cmd.m_Index = -1;

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Attach to Object");
  {
    for (const ezDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Attach to object failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();
}


void ezSceneDocument::DetachFromParent()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  ezMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Detach from Parent");
  {
    for (const ezDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Detach from parent failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();
}

ezStatus ezSceneDocument::CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition)
{
  auto history = GetCommandHistory();

  history->StartTransaction("Create Node");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
  cmdAdd.m_sParentProperty = "Children";
  cmdAdd.m_Index = -1;

  ezUuid NewNode;

  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty() || !bAttachToParent)
  {
    cmdAdd.m_NewObjectGuid.CreateNewUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }
  else
  {
    cmdAdd.m_NewObjectGuid.CreateNewUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    cmdAdd.m_Parent = Sel[0]->GetGuid();
    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (!bAttachToParent && bAtPickedPosition && ctxt.m_pLastPickingResult && !ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
  {
    ezVec3 position = ctxt.m_pLastPickingResult->m_vPickedPosition;

    ezSnapProvider::SnapTranslation(position);

    ezSetObjectPropertyCommand cmdSet;
    cmdSet.m_NewValue = position;
    cmdSet.m_Object = NewNode;
    cmdSet.m_sProperty = "LocalPosition";

    auto res = history->AddCommand(cmdSet);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  // Add a dummy shape icon component, which enables picking
  {
    ezAddObjectCommand cmdAdd;
    cmdAdd.m_pType = ezRTTI::FindTypeByName("ezShapeIconComponent");
    cmdAdd.m_sParentProperty = "Components";
    cmdAdd.m_Index = -1;
    cmdAdd.m_Parent = NewNode;

    auto res = history->AddCommand(cmdAdd);
  }

  history->FinishTransaction();

  GetSelectionManager()->SetSelection(GetObjectManager()->GetObject(NewNode));
  return ezStatus(EZ_SUCCESS);
}

void ezSceneDocument::DuplicateSelection()
{
  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  ezStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", ezConversionUtils::ToString(it.Key(), tmp1), ezConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1); // null terminate

  ezDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Selection");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

void ezSceneDocument::ShowOrHideSelectedObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  auto sel = GetSelectionManager()->GetSelection();

  for (auto pItem : sel)
  {
    if (!pItem->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      continue;

    ApplyRecursive(pItem, [this, bHide](const ezDocumentObject* pObj) {
      // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      // return;

      auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(pObj->GetGuid());
      if (pMeta->m_bHidden != bHide)
      {
        pMeta->m_bHidden = bHide;
        m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::HiddenFlag);
      }
      else
        m_DocumentObjectMetaData.EndModifyMetaData(0);
    });
  }
}

void ezSceneDocument::HideUnselectedObjects()
{
  ShowOrHideAllObjects(ShowOrHide::Hide);

  ShowOrHideSelectedObjects(ShowOrHide::Show);
}

void ezSceneDocument::SetGameMode(GameMode::Enum mode)
{
  if (m_GameMode == mode)
    return;

  // store settings of recently active mode
  m_GameModeData[m_GameMode] = m_CurrentMode;

  m_GameMode = mode;

  switch (m_GameMode)
  {
    case GameMode::Off:
      ShowDocumentStatus("Game Mode: Off");
      break;
    case GameMode::Simulate:
      ShowDocumentStatus("Game Mode: Simulate");
      break;
    case GameMode::Play:
      ShowDocumentStatus("Game Mode: Play");
      break;
  }

  SetRenderSelectionOverlay(m_GameModeData[m_GameMode].m_bRenderSelectionOverlay);
  SetRenderShapeIcons(m_GameModeData[m_GameMode].m_bRenderShapeIcons);
  SetRenderVisualizers(m_GameModeData[m_GameMode].m_bRenderVisualizers);

  if (m_GameMode == GameMode::Off)
  {
    // reset the game world
    SendGameWorldToEngine();
  }

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::GameModeChanged;
  m_GameObjectEvents.Broadcast(e);

  ScheduleSendObjectSelection();
}


void ezSceneDocument::StartSimulateWorld()
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  SetGameMode(GameMode::Simulate);
}


void ezSceneDocument::TriggerGameModePlay(bool bUsePickedPositionAsStart)
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    ezGameObjectEvent e;
    e.m_Type = ezGameObjectEvent::Type::BeforeTriggerGameModePlay;
    m_GameObjectEvents.Broadcast(e);
  }

  UpdateObjectDebugTargets();

  // attempt to start PTG
  // do not change state here
  {
    ezGameModeMsgToEngine msg;
    msg.m_bEnablePTG = true;
    msg.m_bUseStartPosition = false;

    if (bUsePickedPositionAsStart)
    {
      const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

      if (ctxt.m_pLastHoveredViewWidget != nullptr && ctxt.m_pLastHoveredViewWidget->GetDocumentWindow()->GetDocument() == this)
      {
        msg.m_bUseStartPosition = true;
        msg.m_vStartPosition = ctxt.m_pLastPickingResult->m_vPickedPosition;

        ezVec3 vPickDir = ctxt.m_pLastPickingResult->m_vPickedPosition - ctxt.m_pLastPickingResult->m_vPickingRayStart;
        vPickDir.z = 0;
        vPickDir.NormalizeIfNotZero(ezVec3(1, 0, 0));

        msg.m_vStartDirection = vPickDir;
      }
    }

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  {
    ezGameObjectEvent e;
    e.m_Type = ezGameObjectEvent::Type::TriggerGameModePlay;
    m_GameObjectEvents.Broadcast(e);
  }
}


bool ezSceneDocument::StopGameMode()
{
  if (m_GameMode == GameMode::Off)
    return false;

  if (m_GameMode == GameMode::Simulate)
  {
    // we can set that state immediately
    SetGameMode(GameMode::Off);
  }

  if (m_GameMode == GameMode::Play)
  {
    // attempt to stop PTG
    // do not change any state, that will be done by the response msg
    {
      ezGameModeMsgToEngine msg;
      msg.m_bEnablePTG = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    ezGameObjectEvent e;
    e.m_Type = ezGameObjectEvent::Type::TriggerStopGameModePlay;
    m_GameObjectEvents.Broadcast(e);
  }

  return true;
}

void ezSceneDocument::ShowOrHideAllObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  ApplyRecursive(GetObjectManager()->GetRootObject(), [this, bHide](const ezDocumentObject* pObj) {
    // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    // return;

    ezUInt32 uiFlags = 0;

    auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(pObj->GetGuid());

    if (pMeta->m_bHidden != bHide)
    {
      pMeta->m_bHidden = bHide;
      uiFlags = ezDocumentObjectMetaData::HiddenFlag;
    }

    m_DocumentObjectMetaData.EndModifyMetaData(uiFlags);
  });
}
void ezSceneDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.ezAbstractGraph");
}

bool ezSceneDocument::CopySelectedObjects(ezAbstractObjectGraph& graph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.ezAbstractGraph";
  return CopySelectedObjects(graph, nullptr);
}

bool ezSceneDocument::CopySelectedObjects(ezAbstractObjectGraph& graph, ezMap<ezUuid, ezUuid>* out_pParents) const
{
  if (GetSelectionManager()->GetSelection().GetCount() == 0)
    return false;

  // Serialize selection to graph
  auto Selection = GetSelectionManager()->GetTopLevelSelection();

  ezDocumentObjectConverterWriter writer(&graph, GetObjectManager());

  // TODO: objects are required to be named root but this is not enforced or obvious by the interface.
  for (auto item : Selection)
    writer.AddObjectToGraph(item, "root");

  if (out_pParents != nullptr)
  {
    out_pParents->Clear();

    for (auto item : Selection)
    {
      (*out_pParents)[item->GetGuid()] = item->GetParent()->GetGuid();
    }
  }

  AttachMetaDataBeforeSaving(graph);

  return true;
}

bool ezSceneDocument::PasteAt(const ezArrayPtr<PasteInfo>& info, const ezVec3& vPasteAt)
{
  ezVec3 vAvgPos(0.0f);

  for (const PasteInfo& pi : info)
  {
    if (pi.m_pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
      return false;

    vAvgPos += pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<ezVec3>();
  }

  vAvgPos /= info.GetCount();

  for (const PasteInfo& pi : info)
  {
    const ezVec3 vLocalPos = pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<ezVec3>();
    pi.m_pObject->GetTypeAccessor().SetValue("LocalPosition", vLocalPos - vAvgPos + vPasteAt);

    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }
  }

  return true;
}

bool ezSceneDocument::PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info)
{
  for (const PasteInfo& pi : info)
  {
    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }
  }

  return true;
}

bool ezSceneDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition,
                            const char* szMimeType)
{
  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (bAllowPickedPosition && ctxt.m_pLastPickingResult && ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
  {
    if (!PasteAt(info, ctxt.m_pLastPickingResult->m_vPickedPosition))
      return false;
  }
  else
  {
    if (!PasteAtOrignalPosition(info))
      return false;
  }

  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  {
    auto pSelMan = GetSelectionManager();

    ezDeque<const ezDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

bool ezSceneDocument::DuplicateSelectedObjects(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph,
                                               bool bSetSelected)
{
  if (!PasteAtOrignalPosition(info))
    return false;

  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  if (bSetSelected)
  {
    auto pSelMan = GetSelectionManager();

    ezDeque<const ezDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

void ezSceneDocument::EnsureSettingsObjectExist()
{
  auto pRoot = GetObjectManager()->GetRootObject();
  // Use the ezObjectDirectAccessor instead of calling GetObjectAccessor because we do not want
  // undo ops for this operation.
  ezObjectDirectAccessor accessor(GetObjectManager());
  ezVariant value;
  EZ_VERIFY(accessor.ezObjectAccessorBase::GetValue(pRoot, "Settings", value).Succeeded(),
            "The scene doc root should have a settings property.");
  ezUuid id = value.Get<ezUuid>();
  if (!id.IsValid())
  {
    EZ_VERIFY(accessor.ezObjectAccessorBase::AddObject(pRoot, "Settings", ezVariant(), ezGetStaticRTTI<ezSceneDocumentSettings>(), id)
                  .Succeeded(),
              "Adding scene settings object to root failed.");
  }
}

const ezDocumentObject* ezSceneDocument::GetSettingsObject() const
{
  auto pRoot = GetObjectManager()->GetRootObject();
  ezVariant value;
  EZ_VERIFY(GetObjectAccessor()->GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  ezUuid id = value.Get<ezUuid>();
  return GetObjectManager()->GetObject(id);
}

const ezSceneDocumentSettings* ezSceneDocument::GetSettings() const
{
  return static_cast<const ezSceneDocumentSettings*>(m_ObjectMirror.GetNativeObjectPointer(GetSettingsObject()));
}

ezStatus ezSceneDocument::CreateExposedProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index,
                                                ezExposedSceneProperty& out_key) const
{
  const ezDocumentObject* pNodeComponent = ezObjectPropertyPath::FindParentNodeComponent(pObject);
  if (!pObject)
    return ezStatus("No parent node or component found.");

  ezObjectPropertyPathContext context = {pNodeComponent, GetObjectAccessor(), "Children"};
  ezPropertyReference propertyRef = {pObject->GetGuid(), pProperty, index};
  ezStringBuilder sPropertyPath;
  ezStatus res = ezObjectPropertyPath::CreatePropertyPath(context, propertyRef, sPropertyPath);
  if (res.Failed())
    return res;

  out_key.m_Object = pNodeComponent->GetGuid();
  out_key.m_sPropertyPath = sPropertyPath;
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneDocument::AddExposedParameter(const char* szName, const ezDocumentObject* pObject, const ezAbstractProperty* pProperty,
                                              ezVariant index)
{
  if (FindExposedParameter(pObject, pProperty, index) != -1)
    return ezStatus("Exposed parameter already exists.");

  ezExposedSceneProperty key;
  ezStatus res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return res;

  ezUuid id;
  res = GetObjectAccessor()->AddObject(GetSettingsObject(), "ExposedProperties", -1, ezGetStaticRTTI<ezExposedSceneProperty>(), id);
  if (res.Failed())
    return res;
  const ezDocumentObject* pParam = GetObjectManager()->GetObject(id);
  GetObjectAccessor()->SetValue(pParam, "Name", szName).LogFailure();
  GetObjectAccessor()->SetValue(pParam, "Object", key.m_Object).LogFailure();
  GetObjectAccessor()->SetValue(pParam, "PropertyPath", key.m_sPropertyPath).LogFailure();
  return ezStatus(EZ_SUCCESS);
}

ezInt32 ezSceneDocument::FindExposedParameter(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index)
{
  ezExposedSceneProperty key;
  ezStatus res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return -1;

  const ezSceneDocumentSettings* settings = GetSettings();
  for (ezUInt32 i = 0; i < settings->m_ExposedProperties.GetCount(); i++)
  {
    const auto& param = settings->m_ExposedProperties[i];
    if (param.m_Object == key.m_Object && param.m_sPropertyPath == key.m_sPropertyPath)
      return (ezInt32)i;
  }
  return -1;
}

ezStatus ezSceneDocument::RemoveExposedParameter(ezInt32 index)
{
  ezVariant value;
  auto res = GetObjectAccessor()->GetValue(GetSettingsObject(), "ExposedProperties", value, index);
  if (res.Failed())
    return res;

  ezUuid id = value.Get<ezUuid>();
  return GetObjectAccessor()->RemoveObject(GetObjectManager()->GetObject(id));
}


void ezSceneDocument::StoreFavouriteCamera(ezUInt8 uiSlot)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  ezQuadViewPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavouriteCamera[uiSlot];

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView)
  {
    const auto& camera = pView->m_pViewConfig->m_Camera;

    cam.m_vCamPos = camera.GetCenterPosition();
    cam.m_vCamDir = camera.GetCenterDirForwards();
    cam.m_vCamUp = camera.GetCenterDirUp();

    // make sure the data gets saved
    pPreferences->TriggerPreferencesChangedEvent();
  }
}

void ezSceneDocument::RestoreFavouriteCamera(ezUInt8 uiSlot)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  ezQuadViewPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavouriteCamera[uiSlot];

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView)
  {
    pView->InterpolateCameraTo(cam.m_vCamPos, cam.m_vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &cam.m_vCamUp);
  }
}

ezResult ezSceneDocument::JumpToLevelCamera(ezUInt8 uiSlot)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return EZ_FAILURE;

  auto* pObjMan = GetObjectManager();

  ezHybridArray<ezDocumentObject*, 8> stack;
  stack.PushBack(pObjMan->GetRootObject());

  const ezRTTI* pCamType = ezGetStaticRTTI<ezCameraComponent>();
  const ezDocumentObject* pCamObj = nullptr;

  while (!stack.IsEmpty())
  {
    const ezDocumentObject* pObj = stack.PeekBack();
    stack.PopBack();

    stack.PushBackRange(pObj->GetChildren());

    if (pObj->GetType() == pCamType)
    {
      ezInt32 iShortcut = pObj->GetTypeAccessor().GetValue("EditorShortcut").ConvertTo<ezInt32>();

      if (iShortcut == uiSlot)
      {
        pCamObj = pObj->GetParent();
        break;
      }
    }
  }

  if (pCamObj == nullptr)
    return EZ_FAILURE;

  const ezTransform tCam = GetGlobalTransform(pCamObj);

  ezVec3 vUp = tCam.m_qRotation * ezVec3(0, 0, 1);
  pView->InterpolateCameraTo(tCam.m_vPosition, tCam.m_qRotation * ezVec3(1, 0, 0), pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vUp);

  return EZ_SUCCESS;
}

void ezSceneDocument::CreateLevelCamera(ezUInt8 uiSlot)
{
  EZ_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return;

  const auto* pRootObj = GetObjectManager()->GetRootObject();

  const ezVec3 vPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();
  const ezVec3 vDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards().GetNormalized();
  const ezVec3 vUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp().GetNormalized();

  auto* pAccessor = GetObjectAccessor();
  pAccessor->StartTransaction("Create Level Camera");

  ezUuid camObjGuid;
  if (pAccessor->AddObject(pRootObj, "Children", -1, ezGetStaticRTTI<ezGameObject>(), camObjGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return;
  }

  ezMat3 mRot;
  mRot.SetColumn(0, vDir);
  mRot.SetColumn(1, vUp.CrossRH(vDir).GetNormalized());
  mRot.SetColumn(2, vUp);
  ezQuat qRot;
  qRot.SetFromMat3(mRot);
  qRot.Normalize();

  SetGlobalTransform(pAccessor->GetObject(camObjGuid), ezTransform(vPos, qRot),
                     TransformationChanges::Translation | TransformationChanges::Rotation);

  ezUuid camCompGuid;
  if (pAccessor->AddObject(pAccessor->GetObject(camObjGuid), "Components", -1, ezGetStaticRTTI<ezCameraComponent>(), camCompGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return;
  }

  if (pAccessor->SetValue(pAccessor->GetObject(camCompGuid), "EditorShortcut", uiSlot).Failed())
  {
    pAccessor->CancelTransaction();
    return;
  }

  pAccessor->FinishTransaction();
}

void ezSceneDocument::DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & ezDocumentObjectMetaData::HiddenFlag) != 0)
  {
    ezObjectTagMsgToEngine msg;
    msg.m_bSetTag = e.m_pValue->m_bHidden;
    msg.m_sTag = "EditorHidden";
    msg.m_bApplyOnAllChildren = true;

    SendObjectMsg(GetObjectManager()->GetObject(e.m_ObjectKey), &msg);
  }
}

void ezSceneDocument::EngineConnectionEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
      SetGameMode(GameMode::Off);
      break;
  }
}


void ezSceneDocument::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectConfigChanged:
    {
      // we are lazy and just re-select everything here
      // that ensures that ui elements will rebuild their content

      ezDeque<const ezDocumentObject*> selection = GetSelectionManager()->GetSelection();
      GetSelectionManager()->SetSelection(selection);
    }
    break;
  }
}

void ezSceneDocument::HandleGameModeMsg(const ezGameModeMsgToEditor* pMsg)
{
  if (m_GameMode == GameMode::Simulate)
  {
    if (pMsg->m_bRunningPTG)
    {
      m_GameMode = GameMode::Off;
      ezLog::Warning("Incorrect state change from 'simulate' to 'play-the-game'");
    }
    else
    {
      // probably the message just arrived late ?
      return;
    }
  }

  if (m_GameMode == GameMode::Off || m_GameMode == GameMode::Play)
  {
    SetGameMode(pMsg->m_bRunningPTG ? GameMode::Play : GameMode::Off);
    return;
  }

  EZ_REPORT_FAILURE("Unreachable Code reached.");
}


void ezSceneDocument::HandleVisualScriptActivityMsg(const ezVisualScriptActivityMsgToEditor* pMsg)
{
  BroadcastInterDocumentMessage(const_cast<ezVisualScriptActivityMsgToEditor*>(pMsg), this);
}

void ezSceneDocument::HandleObjectStateFromEngineMsg(const ezPushObjectStateMsgToEditor* pMsg)
{
  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Pull Object State");

  for (const auto& state : pMsg->m_ObjectStates)
  {
    auto pObject = GetObjectManager()->GetObject(state.m_ObjectGuid);

    if (pObject)
    {
      SetGlobalTransform(pObject, ezTransform(state.m_vPosition, state.m_qRotation),
                         TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }

  pHistory->FinishTransaction();
}

void ezSceneDocument::SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg)
{
  // if ezObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);
}
void ezSceneDocument::SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg)
{
  // if ezObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);

  for (auto pChild : pObj->GetChildren())
  {
    SendObjectMsgRecursive(pChild, pMsg);
  }
}

void ezSceneDocument::GatherObjectsOfType(ezDocumentObject* pRoot, ezGatherObjectsOfTypeMsgInterDoc* pMsg) const
{
  if (pRoot->GetType() == pMsg->m_pType)
  {
    ezStringBuilder sFullPath;
    GenerateFullDisplayName(pRoot, sFullPath);

    auto& res = pMsg->m_Results.ExpandAndGetRef();
    res.m_ObjectGuid = pRoot->GetGuid();
    res.m_pDocument = this;
    res.m_sDisplayName = sFullPath;
  }

  for (auto pChild : pRoot->GetChildren())
  {
    GatherObjectsOfType(pChild, pMsg);
  }
}

void ezSceneDocument::OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender)
{
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<ezGatherObjectsOfTypeMsgInterDoc>())
  {
    GatherObjectsOfType(GetObjectManager()->GetRootObject(), static_cast<ezGatherObjectsOfTypeMsgInterDoc*>(pMessage));
  }
}

ezStatus ezSceneDocument::RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header)
{
  if (GetGameMode() != GameMode::Off)
    return ezStatus("Cannot export while the scene is simulating");

  EZ_SUCCEED_OR_RETURN(SaveDocument());

  const ezStatus status = ezAssetDocument::RemoteExport(header, szTargetFile);

  // make sure the world is reset
  SendGameWorldToEngine();

  return status;
}

const char* ezSceneDocument::QueryAssetType() const
{
  if (m_bIsPrefab)
    return "Prefab";

  return "Scene";
}

void ezSceneDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // scenes do not have exposed parameters
  if (!m_bIsPrefab)
    return;

  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);

  {
    ezSet<ezString> alreadyExposed;

    auto pSettings = GetSettings();
    for (auto prop : pSettings->m_ExposedProperties)
    {
      auto pRootObject = GetObjectManager()->GetObject(prop.m_Object);
      if (!pRootObject)
      {
        ezLog::Warning("The exposed scene property '{0}' does not point to a valid object and is skipped.", prop.m_sName);
        continue;
      }

      ezObjectPropertyPathContext context = {pRootObject, GetObjectAccessor(), "Children"};

      ezPropertyReference key;
      auto res = ezObjectPropertyPath::ResolvePropertyPath(context, prop.m_sPropertyPath, key);
      if (res.Failed())
      {
        ezLog::Warning("The exposed scene property '{0}' can no longer be resolved and is skipped.", prop.m_sName);
        continue;
      }
      ezVariant value;

      auto pLeafObject = GetObjectManager()->GetObject(key.m_Object);
      if (const ezExposedParametersAttribute* pAttrib = key.m_pProperty->GetAttributeByType<ezExposedParametersAttribute>())
      {
        const ezAbstractProperty* pParameterSourceProp = pLeafObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
        EZ_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'",
                        pAttrib->GetParametersSource(), pLeafObject->GetType()->GetTypeName());

        ezExposedParameterCommandAccessor proxy(context.m_pAccessor, key.m_pProperty, pParameterSourceProp);
        res = proxy.GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
      }
      else
      {
        res = context.m_pAccessor->GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
      }
      EZ_ASSERT_DEBUG(res.Succeeded(), "ResolvePropertyPath succeeded so GetValue should too");

      // do not show the same parameter twice, even if they have different types, as the UI doesn't handle that case properly
      // TODO: we should prevent users from using the same name for differently typed parameters
      // don't do this earlier, we do want to validate each exposed property with the code above
      if (alreadyExposed.Contains(prop.m_sName))
        continue;

      alreadyExposed.Insert(prop.m_sName);

      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = prop.m_sName;
      param->m_sType = key.m_pProperty->GetSpecificType()->GetTypeName();
      param->m_DefaultValue = value;
      for (auto attrib : key.m_pProperty->GetAttributes())
      {
        param->m_Attributes.PushBack(ezReflectionSerializer::Clone(attrib));
      }
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

ezStatus ezSceneDocument::ExportScene(bool bCreateThumbnail)
{
  auto saveres = SaveDocument();

  if (saveres.m_Result.Failed())
    return saveres;

  ezStatus res;

  if (bCreateThumbnail)
  {
    // this is needed to generate a scene thumbnail, however that has a larger overhead (1 sec or so)
    res = ezAssetCurator::GetSingleton()->TransformAsset(GetGuid(), true);
  }
  else
    res = TransformAsset(true);

  if (res.m_Result.Failed())
    ezLog::Error(res.m_sMessage);
  else
    ezLog::Success(res.m_sMessage);

  ShowDocumentStatus(res.m_sMessage.GetData());

  return res;
}

void ezSceneDocument::ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode, const ezMat3& mTransform)
{
  ezExportSceneGeometryMsgToEngine msg;
  msg.m_sOutputFile = szFile;
  msg.m_bSelectionOnly = bOnlySelection;
  msg.m_iExtractionMode = iExtractionMode;
  msg.m_Transform = mTransform;

  SendMessageToEngine(&msg);

  ezQtUiServices::GetSingleton()->ShowAllDocumentsStatusBarMessage(ezFmt("Geometry exported to '{0}'", szFile), ezTime::Seconds(5.0f));
}

void ezSceneDocument::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  ezGameObjectDocument::HandleEngineMessage(pMsg);

  if (const ezGameModeMsgToEditor* msg = ezDynamicCast<const ezGameModeMsgToEditor*>(pMsg))
  {
    HandleGameModeMsg(msg);
    return;
  }

  if (const ezVisualScriptActivityMsgToEditor* msg = ezDynamicCast<const ezVisualScriptActivityMsgToEditor*>(pMsg))
  {
    HandleVisualScriptActivityMsg(msg);
    return;
  }

  if (const ezDocumentOpenResponseMsgToEditor* msg = ezDynamicCast<const ezDocumentOpenResponseMsgToEditor*>(pMsg))
  {
    SyncObjectHiddenState();
  }

  if (const ezPushObjectStateMsgToEditor* msg = ezDynamicCast<const ezPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg(msg);
  }
}

ezStatus ezSceneDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                 const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezSceneDocumentSettings* pSettings = GetSettings();

  ezExposedDocumentObjectPropertiesMsgToEngine msg;
  msg.m_Properties = pSettings->m_ExposedProperties;

  SendMessageToEngine(&msg);

  return RequestExportScene(szTargetFile, AssetHeader);
}


ezStatus ezSceneDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                 const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  /* this function is never called */
  return ezStatus(EZ_FAILURE);
}


ezStatus ezSceneDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);

  // if we were to do this BEFORE making the screenshot, the scene would be killed, but not immediately restored
  // and the screenshot would end up empty
  // instead the engine side ensures simulation is stopped and makes a screenshot of whatever state is visible
  // but then the editor and engine state are out of sync, so AFTER the screenshot is done,
  // we ensure to also stop simulation on the editor side
  StopGameMode();

  return status;
}

void ezSceneDocument::SyncObjectHiddenState()
{
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void ezSceneDocument::SyncObjectHiddenState(ezDocumentObject* pObject)
{
  const bool bHidden = m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid())->m_bHidden;
  m_DocumentObjectMetaData.EndReadMetaData();

  ezObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHidden;
  msg.m_sTag = "EditorHidden";

  SendObjectMsg(pObject, &msg);

  for (auto pChild : pObject->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void ezSceneDocument::UpdateObjectDebugTargets()
{
  ezGatherObjectsForDebugVisMsgInterDoc msg;
  BroadcastInterDocumentMessage(&msg, this);

  {
    ezObjectsForDebugVisMsgToEngine msgToEngine;
    msgToEngine.m_Objects.SetCountUninitialized(sizeof(ezUuid) * msg.m_Objects.GetCount());

    ezMemoryUtils::Copy<ezUInt8>(msgToEngine.m_Objects.GetData(), reinterpret_cast<ezUInt8*>(msg.m_Objects.GetData()),
                                 msgToEngine.m_Objects.GetCount());

    GetEditorEngineConnection()->SendMessage(&msgToEngine);
  }
}
