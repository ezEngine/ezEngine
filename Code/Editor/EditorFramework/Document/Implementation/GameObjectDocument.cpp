#include <EditorFramework/EditorFrameworkPCH.h>

#include "EditorFramework/Panels/LogPanel/LogPanel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectMetaData, 1, ezRTTINoAllocator)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  //EZ_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
  //}
  //EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEvent<const ezGameObjectDocumentEvent&> ezGameObjectDocument::s_GameObjectDocumentEvents;

ezGameObjectDocument::ezGameObjectDocument(ezStringView sDocumentPath, ezDocumentObjectManager* pObjectManager, ezAssetDocEngineConnection engineConnectionType)
  : ezAssetDocument(sDocumentPath, pObjectManager, engineConnectionType)
{
  using Meta = ezObjectMetaData<ezUuid, ezGameObjectMetaData>;
  m_GameObjectMetaData = EZ_DEFAULT_NEW(Meta);

  EZ_ASSERT_DEV(engineConnectionType == ezAssetDocEngineConnection::FullObjectMirroring,
    "ezGameObjectDocument only supports full mirroring engine connection types. The parameter only exists for interface compatibility.");

  m_CurrentMode.m_bRenderSelectionOverlay = true;
  m_CurrentMode.m_bRenderShapeIcons = true;
  m_CurrentMode.m_bRenderVisualizers = true;
}

ezGameObjectDocument::~ezGameObjectDocument()
{
  UnsubscribeGameObjectEventHandlers();
  DeallocateEditTools();
}

void ezGameObjectDocument::SubscribeGameObjectEventHandlers()
{
  m_SelectionManagerEventHandlerID = GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectDocument::SelectionManagerEventHandler, this));
  m_ObjectPropertyEventHandlerID = GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectDocument::ObjectPropertyEventHandler, this));
  m_ObjectStructureEventHandlerID = GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectDocument::ObjectStructureEventHandler, this));
  m_ObjectEventHandlerID = GetObjectManager()->m_ObjectEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectDocument::ObjectEventHandler, this));

  s_GameObjectDocumentEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void ezGameObjectDocument::UnsubscribeGameObjectEventHandlers()
{
  GetSelectionManager()->m_Events.RemoveEventHandler(m_SelectionManagerEventHandlerID);
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(m_ObjectPropertyEventHandlerID);
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(m_ObjectStructureEventHandlerID);
  GetObjectManager()->m_ObjectEvents.RemoveEventHandler(m_ObjectEventHandlerID);

  s_GameObjectDocumentEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void ezGameObjectDocument::GameObjectDocumentEventHandler(const ezGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    // case ezGameObjectDocumentEvent::Type::GameMode_StartingExternal: // the external player doesn't log to the editor panel, so don't need to clear that
    case ezGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case ezGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      auto pEditorPrefsUser = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
      if (pEditorPrefsUser && pEditorPrefsUser->m_bClearEditorLogsOnPlay)
      {
        ezQtLogPanel::GetSingleton()->CombinedLog->GetLog()->Clear();

        // on play, the engine log has a lot of activity, so makes sense to clear that first
        ezQtLogPanel::GetSingleton()->EngineLog->GetLog()->Clear();

        // but I think we usually want to keep the editor log around
        // ezQtLogPanel::GetSingleton()->EditorLog->GetLog()->Clear();
      }
    }
    break;
    default:
      break;
  }
}

ezEditorInputContext* ezGameObjectDocument::GetEditorInputContextOverride()
{
  if (GetActiveEditTool() && GetActiveEditTool()->GetEditorInputContextOverride() != nullptr)
  {
    return GetActiveEditTool()->GetEditorInputContextOverride();
  }

  return nullptr;
}

void ezGameObjectDocument::SetEditToolConfigDelegate(ezDelegate<void(ezGameObjectEditTool*)> configDelegate)
{
  m_EditToolConfigDelegate = configDelegate;
}

bool ezGameObjectDocument::IsActiveEditTool(const ezRTTI* pEditToolType) const
{
  if (m_pActiveEditTool == nullptr)
    return pEditToolType == nullptr;

  if (pEditToolType == nullptr)
    return false;

  return m_pActiveEditTool->IsInstanceOf(pEditToolType);
}

void ezGameObjectDocument::SetActiveEditTool(const ezRTTI* pEditToolType)
{
  ezGameObjectEditTool* pEditTool = nullptr;

  if (pEditToolType != nullptr)
  {
    auto it = m_CreatedEditTools.Find(pEditToolType);
    if (it.IsValid())
    {
      pEditTool = it.Value();
    }
    else
    {
      EZ_ASSERT_DEBUG(m_EditToolConfigDelegate.IsValid(), "Window did not specify a delegate to configure edit tools");

      pEditTool = pEditToolType->GetAllocator()->Allocate<ezGameObjectEditTool>();
      m_CreatedEditTools[pEditToolType] = pEditTool;

      m_EditToolConfigDelegate(pEditTool);
    }
  }

  if (m_pActiveEditTool == pEditTool)
  {
    if (m_pActiveEditTool == nullptr)
    {
      // if there is currently no active edit tool, we may still have a manipulator active
      // if so, when repeatedly selecting this action, toggle the visibility of manipulators
      ezManipulatorManager::GetSingleton()->ToggleHideActiveManipulator(this);
    }

    return;
  }

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(false);

  m_pActiveEditTool = pEditTool;

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(true);

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);
}

void ezGameObjectDocument::SetAddAmbientLight(bool b)
{
  if (m_bAddAmbientLight == b)
    return;

  m_bAddAmbientLight = b;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::AddAmbientLightChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Ambient Light: {}", m_bAddAmbientLight ? "ON" : "OFF"));
}

void ezGameObjectDocument::SetPickTransparent(bool b)
{
  if (m_bPickTransparent == b)
    return;

  m_bPickTransparent = b;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::PickTransparentChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Select Transparent: {}", m_bPickTransparent ? "ON" : "OFF"));

  if (m_bPickTransparent == false)
  {
    // make sure no transparent object is currently selected
    GetSelectionManager()->Clear();
  }
}

void ezGameObjectDocument::SetActiveParent(ezUuid object)
{
  if (m_ActiveParent != object)
  {
    if (auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(m_ActiveParent))
    {
      m_DocumentObjectMetaData->EndModifyMetaData(ezDocumentObjectMetaData::ActiveParentFlag);
    }

    m_ActiveParent = object;

    if (auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(m_ActiveParent))
    {
      m_DocumentObjectMetaData->EndModifyMetaData(ezDocumentObjectMetaData::ActiveParentFlag);
    }
  }
}

void ezGameObjectDocument::SetGizmoWorldSpace(bool bWorldSpace)
{
  if (m_bGizmoWorldSpace == bWorldSpace)
    return;

  m_bGizmoWorldSpace = bWorldSpace;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Transform in {}", m_bGizmoWorldSpace ? "World Space" : "Object Space"));
}

bool ezGameObjectDocument::GetGizmoWorldSpace() const
{
  return m_bGizmoWorldSpace;
}

void ezGameObjectDocument::SetGizmoMoveParentOnly(bool bMoveParent)
{
  if (m_bGizmoMoveParentOnly == bMoveParent)
    return;

  m_bGizmoMoveParentOnly = bMoveParent;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Move Parent Only: {}", m_bGizmoMoveParentOnly ? "ON" : "OFF"));
}

void ezGameObjectDocument::DetermineNodeName(const ezDocumentObject* pObject, const ezUuid& prefabGuid, ezStringBuilder& out_sResult, QIcon* out_pIcon /*= nullptr*/) const
{
  // tries to find a good name for a node by looking at the attached components and their properties

  bool bHasIcon = false;

  if (prefabGuid.IsValid())
  {
    auto pInfo = ezAssetCurator::GetSingleton()->GetSubAsset(prefabGuid);

    if (pInfo)
    {
      ezStringBuilder sPath = pInfo->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      sPath = sPath.GetFileName();

      out_sResult.Set("Prefab: ", sPath);
    }
    else
      out_sResult = "Prefab: Invalid Asset";
  }

  const bool bHasChildren = pObject->GetTypeAccessor().GetCount("Children") > 0;

  ezStringBuilder tmp;

  const ezInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
  for (ezInt32 i = 0; i < iComponents; i++)
  {
    ezVariant value = pObject->GetTypeAccessor().GetValue("Components", i);
    auto pChild = GetObjectManager()->GetObject(value.Get<ezUuid>());
    EZ_ASSERT_DEBUG(pChild->GetTypeAccessor().GetType()->IsDerivedFrom<ezComponent>(), "Non-component found in component set.");
    // take the first components name
    if (!bHasIcon && out_pIcon != nullptr)
    {
      bHasIcon = true;

      ezColor color = ezColor::MakeZero();

      if (auto pCatAttr = pChild->GetTypeAccessor().GetType()->GetAttributeByType<ezCategoryAttribute>())
      {
        color = ezColorScheme::GetCategoryColor(pCatAttr->GetCategory(), ezColorScheme::CategoryColorUsage::SceneTreeIcon);
      }

      ezStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pChild->GetTypeAccessor().GetType()->GetTypeName(), ".svg");
      *out_pIcon = ezQtUiServices::GetCachedIconResource(sIconName.GetData(), color);
    }

    if (out_sResult.IsEmpty())
    {
      // try to translate the component name, that will typically make it a nice clean name already
      out_sResult = ezTranslate(pChild->GetTypeAccessor().GetType()->GetTypeName().GetData(tmp));

      // if no translation is available, clean up the component name in a simple way
      if (out_sResult.EndsWith_NoCase("Component"))
        out_sResult.Shrink(0, 9);
      if (out_sResult.StartsWith("ez"))
        out_sResult.Shrink(2, 0);

      if (auto pInDev = pChild->GetTypeAccessor().GetType()->GetAttributeByType<ezInDevelopmentAttribute>())
      {
        out_sResult.AppendFormat(" [ {} ]", pInDev->GetString());
      }
    }

    if (prefabGuid.IsValid())
      continue;

    const auto& properties = pChild->GetTypeAccessor().GetType()->GetProperties();

    for (auto pProperty : properties)
    {
      const auto type = pProperty->GetSpecificType();

      // search for string properties that also have an asset browser property -> they reference an asset, so this is most likely the most
      // relevant property
      if ((type == ezGetStaticRTTI<const char*>() || type == ezGetStaticRTTI<ezString>() || type == ezGetStaticRTTI<ezStringView>()) && pProperty->GetAttributeByType<ezAssetBrowserAttribute>() != nullptr)
      {
        ezStringBuilder sValue;
        if (pProperty->GetCategory() == ezPropertyCategory::Member)
        {
          sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName()).ConvertTo<ezString>();
        }
        else if (pProperty->GetCategory() == ezPropertyCategory::Array)
        {
          const ezInt32 iCount = pChild->GetTypeAccessor().GetCount(pProperty->GetPropertyName());
          if (iCount > 0)
          {
            sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName(), 0).ConvertTo<ezString>();
          }
        }

        // if the property is a full asset guid reference, convert it to a file name
        if (ezConversionUtils::IsStringUuid(sValue))
        {
          const ezUuid AssetGuid = ezConversionUtils::ConvertStringToUuid(sValue);

          auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

          if (pAsset)
            sValue = pAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          else
            sValue = "<unknown>";
        }

        // only use the file name for our display
        sValue = sValue.GetFileName();

        if (!sValue.IsEmpty())
          out_sResult.Append(": ", sValue);

        return;
      }
    }
  }

  if (!out_sResult.IsEmpty())
    return;

  if (bHasChildren)
    out_sResult = "Group";
  else
    out_sResult = "Object";
}


void ezGameObjectDocument::QueryCachedNodeName(
  const ezDocumentObject* pObject, ezStringBuilder& out_sResult, ezUuid* out_pPrefabGuid, QIcon* out_pIcon /*= nullptr*/) const
{
  auto pMetaScene = m_GameObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  auto pMetaDoc = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  const ezUuid prefabGuid = pMetaDoc->m_CreateFromPrefab;

  if (out_pPrefabGuid != nullptr)
    *out_pPrefabGuid = prefabGuid;

  out_sResult = pMetaScene->m_CachedNodeName;
  if (out_pIcon)
    *out_pIcon = pMetaScene->m_Icon;
  m_GameObjectMetaData->EndReadMetaData();
  m_DocumentObjectMetaData->EndReadMetaData();

  if (out_sResult.IsEmpty())
  {
    // the cached node name is only determined once
    // after that only a node rename (EditRole) will currently trigger a cache cleaning and thus a reevaluation
    // this is to prevent excessive re-computation of the name, which is quite involved

    QIcon icon;
    DetermineNodeName(pObject, prefabGuid, out_sResult, &icon);
    ezString sNodeName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();
    if (!sNodeName.IsEmpty())
    {
      out_sResult = sNodeName;
    }
    auto pMetaWrite = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaWrite->m_CachedNodeName = out_sResult;
    pMetaWrite->m_Icon = icon;
    m_GameObjectMetaData->EndModifyMetaData(0); // no need to broadcast this change

    if (out_pIcon != nullptr)
      *out_pIcon = icon;
  }
}


void ezGameObjectDocument::GenerateFullDisplayName(const ezDocumentObject* pRoot, ezStringBuilder& out_sFullPath) const
{
  if (pRoot == nullptr || pRoot == GetObjectManager()->GetRootObject())
    return;

  GenerateFullDisplayName(pRoot->GetParent(), out_sFullPath);

  if (!pRoot->GetType()->IsDerivedFrom<ezComponent>())
  {
    ezStringBuilder sObjectName;
    QueryCachedNodeName(pRoot, sObjectName);

    out_sFullPath.AppendPath(sObjectName);
  }
}

ezTransform ezGameObjectDocument::GetGlobalTransform(const ezDocumentObject* pObject) const
{
  if (!m_GlobalTransforms.Contains(pObject))
  {
    ComputeGlobalTransform(pObject);
  }

  return ezSimdConversion::ToTransform(m_GlobalTransforms[pObject]);
}

void ezGameObjectDocument::SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 uiTransformationChanges) const
{
  ezObjectAccessorBase* pAccessor = GetObjectAccessor();
  auto pHistory = GetCommandHistory();
  if (!pHistory->IsInTransaction())
  {
    InvalidateGlobalTransformValue(pObject);
    return;
  }

  const ezDocumentObject* pParent = pObject->GetParent();

  ezSimdTransform tLocal;
  ezSimdTransform simdT = ezSimdConversion::ToTransform(t);

  if (pParent != nullptr)
  {
    if (!m_GlobalTransforms.Contains(pParent))
    {
      ComputeGlobalTransform(pParent);
    }

    ezSimdTransform tParent = m_GlobalTransforms[pParent];

    tLocal = ezSimdTransform::MakeLocalTransform(tParent, simdT);
  }
  else
  {
    tLocal = simdT;
  }

  ezVec3 vLocalPos = ezSimdConversion::ToVec3(tLocal.m_Position);
  ezVec3 vLocalScale = ezSimdConversion::ToVec3(tLocal.m_Scale);
  ezQuat qLocalRot = ezSimdConversion::ToQuat(tLocal.m_Rotation);
  float fUniformScale = 1.0f;

  if (vLocalScale.x == vLocalScale.y && vLocalScale.x == vLocalScale.z)
  {
    fUniformScale = vLocalScale.x;
    vLocalScale.Set(1.0f);
  }

  // unfortunately when we are dragging an object the 'temporary' transaction is undone every time before the new commands are sent
  // that means the values that we read here, are always the original values before the object was modified at all
  // therefore when the original position and the new position are identical, that means the user dragged the object to the previous
  // position it does NOT mean that there is no change, in fact there is a change, just back to the original value

  // if (pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>() != vLocalPos)
  if ((uiTransformationChanges & TransformationChanges::Translation) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalPosition", vLocalPos).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>() != qLocalRot)
  if ((uiTransformationChanges & TransformationChanges::Rotation) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalRotation", qLocalRot).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>() != vLocalScale)
  if ((uiTransformationChanges & TransformationChanges::Scale) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalScaling", vLocalScale).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>() != fUniformScale)
  if ((uiTransformationChanges & TransformationChanges::UniformScale) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalUniformScaling", fUniformScale).LogFailure();
  }

  // will be recomputed the next time it is queried
  InvalidateGlobalTransformValue(pObject);
}

void ezGameObjectDocument::SetGlobalTransformParentOnly(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 uiTransformationChanges) const
{
  ezHybridArray<ezTransform, 16> childTransforms;
  const auto& children = pObject->GetChildren();

  childTransforms.SetCountUninitialized(children.GetCount());

  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const ezDocumentObject* pChild = children[i];
    childTransforms[i] = GetGlobalTransform(pChild);
  }

  SetGlobalTransform(pObject, t, uiTransformationChanges);

  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const ezDocumentObject* pChild = children[i];
    SetGlobalTransform(pChild, childTransforms[i], TransformationChanges::All);
  }
}

void ezGameObjectDocument::InvalidateGlobalTransformValue(const ezDocumentObject* pObject) const
{
  // will be recomputed the next time it is queried
  m_GlobalTransforms.Remove(pObject);

  /// \todo If all parents are always inserted as well, we can stop once an object is found that is not in the list

  for (auto pChild : pObject->GetChildren())
  {
    InvalidateGlobalTransformValue(pChild);
  }
}

ezResult ezGameObjectDocument::ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_result) const
{
  const ezDocumentObject* pObj = pObject;

  while (pObj && !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
  {
    pObj = pObj->GetParent();
  }

  if (pObj)
  {
    out_result = ComputeGlobalTransform(pObj);
    return EZ_SUCCESS;
  }
  else
  {
    out_result.SetIdentity();
    return EZ_FAILURE;
  }
}

bool ezGameObjectDocument::GetGizmoMoveParentOnly() const
{
  return m_bGizmoMoveParentOnly;
}

void ezGameObjectDocument::DeallocateEditTools()
{
  for (auto it = m_CreatedEditTools.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }

  m_CreatedEditTools.Clear();
}

void ezGameObjectDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  SubscribeGameObjectEventHandlers();
}


void ezGameObjectDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  ezAssetDocument::AttachMetaDataBeforeSaving(graph);

  m_GameObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void ezGameObjectDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  ezAssetDocument::RestoreMetaDataAfterLoading(graph, bUndoable);

  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void ezGameObjectDocument::TriggerShowSelectionInScenegraph() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::TriggerShowSelectionInScenegraph;
  m_GameObjectEvents.Broadcast(e);
}

void ezGameObjectDocument::TriggerFocusOnSelection(bool bAllViews) const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezGameObjectEvent e;
  e.m_Type = bAllViews ? ezGameObjectEvent::Type::TriggerFocusOnSelection_All : ezGameObjectEvent::Type::TriggerFocusOnSelection_Hovered;
  m_GameObjectEvents.Broadcast(e);
}

void ezGameObjectDocument::TriggerSnapPivotToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void ezGameObjectDocument::TriggerSnapEachObjectToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void ezGameObjectDocument::SnapCameraToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.GetCount() != 1)
    return;

  ezTransform trans;
  if (ComputeObjectTransformation(selection[0], trans).Failed())
    return;

  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const ezCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const ezVec3 vForward = trans.m_qRotation * ezVec3(1, 0, 0);
  const ezVec3 vUp = trans.m_qRotation * ezVec3(0, 0, 1);

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(trans.m_vPosition, vForward, pCamera->GetFovOrDim(), &vUp);
}


void ezGameObjectDocument::MoveCameraHere()
{
  const auto& ctxt = ezQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr)
    return;

  if (ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
    return;

  const ezCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const ezVec3 vCurPos = pCamera->GetCenterPosition();
  const ezVec3 vDirToPos = ctxt.m_pLastPickingResult->m_vPickedPosition - vCurPos;

  // don't move the entire distance, keep some distance to the target position
  ezVec3 vPos = vCurPos + 0.9f * vDirToPos;
  ezVec3 vCamDir = pCamera->GetCenterDirForwards();
  ezVec3 vCamUp = pCamera->GetCenterDirUp();

  // if the projection mode of the view is orthographic, ignore the direction
  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
  {
    const auto& oldCam = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

    vCamDir = oldCam.GetCenterDirForwards();
    vCamUp = oldCam.GetCenterDirUp();

    switch (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective)
    {
      case ezSceneViewPerspective::Orthogonal_Front:
        vPos.x = oldCam.GetCenterPosition().x;
        break;
      case ezSceneViewPerspective::Orthogonal_Right:
        vPos.y = oldCam.GetCenterPosition().y;
        break;
      case ezSceneViewPerspective::Orthogonal_Top:
        vPos.z = oldCam.GetCenterPosition().z;
        break;

      default:
        break;
    }
  }
  else
  {
    // in ortho modes it is fine to move just anywhere, and we often don't pick a real object,
    // because of the wireframe picking

    // however, in perspective modes, don't move, if we haven't picked any real object
    // this happens for example when one picks the sky -> you would end up far away
    if (!ctxt.m_pLastPickingResult->m_PickedComponent.IsValid() && !ctxt.m_pLastPickingResult->m_PickedOther.IsValid())
      return;
  }

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(vPos, vCamDir, pCamera->GetFovOrDim(), &vCamUp);
}

void ezGameObjectDocument::ScheduleSendObjectSelection()
{
  m_iResendSelection = 2;
}

void ezGameObjectDocument::SendGameWorldToEngine()
{
  SendDocumentOpenMessage(true);
}

void ezGameObjectDocument::SetSimulationSpeed(float f)
{
  if (m_fSimulationSpeed == f)
    return;

  m_fSimulationSpeed = f;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::SimulationSpeedChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Simulation Speed: {0}%%", (ezInt32)(m_fSimulationSpeed * 100.0f)));
}

void ezGameObjectDocument::SetRenderSelectionOverlay(bool b)
{
  if (m_CurrentMode.m_bRenderSelectionOverlay == b)
    return;

  m_CurrentMode.m_bRenderSelectionOverlay = b;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::RenderSelectionOverlayChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Selection Overlay: {}", m_CurrentMode.m_bRenderSelectionOverlay ? "ON" : "OFF"));
}


void ezGameObjectDocument::SetRenderVisualizers(bool b)
{
  if (m_CurrentMode.m_bRenderVisualizers == b)
    return;

  m_CurrentMode.m_bRenderVisualizers = b;

  ezVisualizerManager::GetSingleton()->SetVisualizersActive(GetActiveSubDocument(), m_CurrentMode.m_bRenderVisualizers);

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::RenderVisualizersChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Visualizers: {}", m_CurrentMode.m_bRenderVisualizers ? "ON" : "OFF"));
}

void ezGameObjectDocument::SetRenderShapeIcons(bool b)
{
  if (m_CurrentMode.m_bRenderShapeIcons == b)
    return;

  m_CurrentMode.m_bRenderShapeIcons = b;

  ezGameObjectEvent e;
  e.m_Type = ezGameObjectEvent::Type::RenderShapeIconsChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(ezFmt("Shape Icons: {}", m_CurrentMode.m_bRenderShapeIcons ? "ON" : "OFF"));
}

void ezGameObjectDocument::ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "LocalPosition" || e.m_sProperty == "LocalRotation" || e.m_sProperty == "LocalScaling" ||
      e.m_sProperty == "LocalUniformScaling")
  {
    InvalidateGlobalTransformValue(e.m_pObject);
  }

  if (e.m_sProperty == "Name")
  {
    auto pMetaWrite = m_GameObjectMetaData->BeginModifyMetaData(e.m_pObject->GetGuid());
    pMetaWrite->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(ezGameObjectMetaData::CachedName);
  }
}

void ezGameObjectDocument::ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (e.m_pObject && e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
  {
    switch (e.m_EventType)
    {
      case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
      {
        // make sure the cache is filled with a proper value
        GetGlobalTransform(e.m_pObject);
      }
      break;

      case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      {
        // read cached value, hopefully it was not invalidated in between BeforeObjectMoved and AfterObjectMoved
        ezTransform t = GetGlobalTransform(e.m_pObject);

        SetGlobalTransform(e.m_pObject, t, TransformationChanges::All);
      }
      break;

      default:
        break;
    }
  }
  else
  {
    switch (e.m_EventType)
    {
      case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
      case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
        if (e.m_sParentProperty == "Components")
        {
          if (e.m_pPreviousParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pPreviousParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(ezGameObjectMetaData::CachedName);
          }

          if (e.m_pNewParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pNewParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(ezGameObjectMetaData::CachedName);
          }
        }
        break;

      default:
        break;
    }
  }
}


void ezGameObjectDocument::ObjectEventHandler(const ezDocumentObjectEvent& e)
{
  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  switch (e.m_EventType)
  {
    case ezDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      // clean up object meta data upon object destruction, because we can :-P
      if (GetObjectManager()->GetObject(e.m_pObject->GetGuid()) == nullptr)
      {
        // make sure there is no object with this GUID still "added" to the document
        // this can happen if two objects use the same GUID, only one object can be "added" at a time, but multiple objects with the same
        // GUID may exist the same GUID is in use, when a prefab is recreated (updated) and the GUIDs are restored, such that references
        // don't change the object that is being destroyed is typically referenced by a command that was in the redo-queue that got purged

        m_DocumentObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
        m_GameObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
      }
    }
    break;

    default:
      break;
  }
}


void ezGameObjectDocument::SelectionManagerEventHandler(const ezSelectionManagerEvent& e)
{
  ScheduleSendObjectSelection();

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  if (pPreferences->m_bExpandSceneTreeOnSelection)
  {
    TriggerShowSelectionInScenegraph();
  }
}

void ezGameObjectDocument::SendObjectSelection()
{
  if (m_iResendSelection <= 0)
    return;

  --m_iResendSelection;

  const auto& sel = GetSelectionManager()->GetRuntimeOverrideSelection().IsEmpty() ? GetSelectionManager()->GetSelection() : GetSelectionManager()->GetRuntimeOverrideSelection();

  ezObjectSelectionMsgToEngine msg;
  ezStringBuilder sTemp;
  ezStringBuilder sGuid;

  for (const auto& item : sel)
  {
    ezConversionUtils::ToString(item->GetGuid(), sGuid);

    sTemp.Append(";", sGuid);
  }

  msg.m_sSelection = sTemp;

  GetEditorEngineConnection()->SendMessage(&msg);
}
// static
ezTransform ezGameObjectDocument::QueryLocalTransform(const ezDocumentObject* pObject)
{
  const ezVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const ezQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return ezTransform(vTranslation, qRotation, vScaling * fScaling);
}

// static
ezSimdTransform ezGameObjectDocument::QueryLocalTransformSimd(const ezDocumentObject* pObject)
{
  const ezVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const ezQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return ezSimdTransform(ezSimdConversion::ToVec3(vTranslation), ezSimdConversion::ToQuat(qRotation), ezSimdConversion::ToVec3(vScaling * fScaling));
}


ezTransform ezGameObjectDocument::ComputeGlobalTransform(const ezDocumentObject* pObject) const
{
  if (pObject == nullptr || pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
  {
    m_GlobalTransforms[pObject] = ezSimdTransform::MakeIdentity();
    return ezTransform::MakeIdentity();
  }

  const ezSimdTransform tParent = ezSimdConversion::ToTransform(ComputeGlobalTransform(pObject->GetParent()));
  const ezSimdTransform tLocal = QueryLocalTransformSimd(pObject);

  ezSimdTransform tGlobal = ezSimdTransform::MakeGlobalTransform(tParent, tLocal);

  m_GlobalTransforms[pObject] = tGlobal;

  return ezSimdConversion::ToTransform(tGlobal);
}

void ezGameObjectDocument::ComputeTopLevelSelectedGameObjects(ezDeque<ezSelectedGameObject>& out_selection)
{
  // Get the list of all objects that are manipulated
  // and store their original transformation

  out_selection.Clear();

  auto hType = ezGetStaticRTTI<ezGameObject>();

  auto pSelMan = GetSelectionManager();
  const auto& Selection = pSelMan->GetSelection();
  for (ezUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
  {
    if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
      continue;

    // ignore objects, whose parent is already selected as well, so that transformations aren't applied
    // multiple times on the same hierarchy
    if (pSelMan->IsParentSelected(Selection[sel]))
      continue;

    ezSelectedGameObject& sgo = out_selection.ExpandAndGetRef();
    sgo.m_pObject = Selection[sel];
    sgo.m_GlobalTransform = GetGlobalTransform(sgo.m_pObject);
    sgo.m_vLocalScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
    sgo.m_fLocalUniformScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();
  }
}

void ezGameObjectDocument::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  SUPER::HandleEngineMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenResponseMsgToEditor>())
  {
    ScheduleSendObjectSelection();
  }
}
