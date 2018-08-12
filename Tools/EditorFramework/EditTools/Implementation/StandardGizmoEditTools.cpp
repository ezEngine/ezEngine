#include <PCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmoEditTool, 1, ezRTTIDefaultAllocator<ezTranslateGizmoEditTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTranslateGizmoEditTool::ezTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

ezTranslateGizmoEditTool::~ezTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));

  auto& events = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument())->m_ChangedEvent;

  if (events.HasEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::OnPreferenceChange, this)))
    events.RemoveEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::OnPreferenceChange, this));
}

void ezTranslateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_TranslateGizmo.SetOwner(GetWindow(), nullptr);

  ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument())
      ->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::OnPreferenceChange, this));
}

void ezTranslateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_TranslateGizmo.SetVisible(visible);
}

void ezTranslateGizmoEditTool::ApplyGizmoTransformation(const ezTransform& transform)
{
  m_TranslateGizmo.SetTransformation(transform);
}

void ezTranslateGizmoEditTool::TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e)
{
  ezObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
          QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when shift is held while dragging the item
      if (bDuplicate && (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<ezOrthoGizmoContext>()))
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }

      if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
      {
        m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::MouseDiff);
      }
    }
    break;

    case ezGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      ezTransform tNew;

      if (e.m_pGizmo == &m_TranslateGizmo)
      {
        const ezVec3 vTranslate = m_TranslateGizmo.GetTranslationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::MouseDiff);

          auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
          if (pFocusedView != nullptr)
          {
            pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(m_TranslateGizmo.GetTranslationDiff());
          }
        }
        else
        {
          m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::ScreenProjection);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<ezOrthoGizmoContext>())
      {
        const ezOrthoGizmoContext* pOrtho = static_cast<const ezOrthoGizmoContext*>(e.m_pGizmo);

        const ezVec3 vTranslate = pOrtho->GetTranslationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        if (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          // move the camera with the translated object

          auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
          if (pFocusedView != nullptr)
          {
            pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(pOrtho->GetTranslationDiff());
          }
        }
      }

      pAccessor->FinishTransaction();
    }
    break;
  }
}

void ezTranslateGizmoEditTool::OnPreferenceChange(ezPreferences* pref)
{
  ezScenePreferencesUser* pPref = ezDynamicCast<ezScenePreferencesUser*>(pref);

  m_TranslateGizmo.SetCameraSpeed(ezCameraMoveContext::ConvertCameraSpeed(pPref->GetCameraSpeed()));
}

void ezTranslateGizmoEditTool::GetGridSettings(ezGridSettingsMsgToEngine& msg)
{
  auto pSceneDoc = GetDocument();
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument());

  msg.m_fGridDensity =
      ezSnapProvider::GetTranslationSnapValue() * (pSceneDoc->GetGizmoWorldSpace() ? 1.0f : -1.0f); // negative density = local space
  msg.m_vGridTangent1.SetZero();                                                                    // indicates that the grid is disabled
  msg.m_vGridTangent2.SetZero();                                                                    // indicates that the grid is disabled

  ezTranslateGizmo& translateGizmo = m_TranslateGizmo;

  if (pPreferences->GetShowGrid() && translateGizmo.IsVisible())
  {
    msg.m_vGridCenter = translateGizmo.GetStartPosition();

    if (translateGizmo.GetTranslateMode() == ezTranslateGizmo::TranslateMode::Axis)
      msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;

    if (pSceneDoc->GetGizmoWorldSpace())
    {
      ezSnapProvider::SnapTranslation(msg.m_vGridCenter);

      switch (translateGizmo.GetLastPlaneInteraction())
      {
        case ezTranslateGizmo::PlaneInteraction::PlaneX:
          msg.m_vGridCenter.y = ezMath::Round(msg.m_vGridCenter.y, ezSnapProvider::GetTranslationSnapValue() * 10);
          msg.m_vGridCenter.z = ezMath::Round(msg.m_vGridCenter.z, ezSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case ezTranslateGizmo::PlaneInteraction::PlaneY:
          msg.m_vGridCenter.x = ezMath::Round(msg.m_vGridCenter.x, ezSnapProvider::GetTranslationSnapValue() * 10);
          msg.m_vGridCenter.z = ezMath::Round(msg.m_vGridCenter.z, ezSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case ezTranslateGizmo::PlaneInteraction::PlaneZ:
          msg.m_vGridCenter.x = ezMath::Round(msg.m_vGridCenter.x, ezSnapProvider::GetTranslationSnapValue() * 10);
          msg.m_vGridCenter.y = ezMath::Round(msg.m_vGridCenter.y, ezSnapProvider::GetTranslationSnapValue() * 10);
          break;
      }
    }

    switch (translateGizmo.GetLastPlaneInteraction())
    {
      case ezTranslateGizmo::PlaneInteraction::PlaneX:
        msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
        msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneY:
        msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
        msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneZ:
        msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
        msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRotateGizmoEditTool, 1, ezRTTIDefaultAllocator<ezRotateGizmoEditTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRotateGizmoEditTool::ezRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

ezRotateGizmoEditTool::~ezRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void ezRotateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_RotateGizmo.SetOwner(GetWindow(), nullptr);
}

void ezRotateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_RotateGizmo.SetVisible(visible);
}

void ezRotateGizmoEditTool::ApplyGizmoTransformation(const ezTransform& transform)
{
  m_RotateGizmo.SetTransformation(transform);
}

void ezRotateGizmoEditTool::TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e)
{
  ezObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
          QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when shift is held while dragging the item
      if (e.m_pGizmo == &m_RotateGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case ezGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      ezTransform tNew;

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        const ezQuat qRotation = m_RotateGizmo.GetRotationResult();
        const ezVec3 vPivot = m_RotateGizmo.GetTransformation().m_vPosition;

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
          tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew,
                                                    TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<ezOrthoGizmoContext>())
      {
        const ezOrthoGizmoContext* pOrtho = static_cast<const ezOrthoGizmoContext*>(e.m_pGizmo);

        const ezQuat qRotation = pOrtho->GetRotationResult();

        // const ezVec3 vPivot(0);

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
          // tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScaleGizmoEditTool, 1, ezRTTIDefaultAllocator<ezScaleGizmoEditTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezScaleGizmoEditTool::ezScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

ezScaleGizmoEditTool::~ezScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void ezScaleGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_ScaleGizmo.SetOwner(GetWindow(), nullptr);
}

void ezScaleGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_ScaleGizmo.SetVisible(visible);
}

void ezScaleGizmoEditTool::ApplyGizmoTransformation(const ezTransform& transform)
{
  m_ScaleGizmo.SetTransformation(transform);
}

void ezScaleGizmoEditTool::TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e)
{
  ezObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      ezTransform tNew;

      bool bCancel = false;

      if (e.m_pGizmo == &m_ScaleGizmo)
      {
        const ezVec3 vScale = m_ScaleGizmo.GetScalingResult();
        if (vScale.x == vScale.y && vScale.x == vScale.z)
        {
          for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            float fNewScale = obj.m_fLocalUniformScaling * vScale.x;

            if (pAccessor->SetValue(obj.m_pObject, "LocalUniformScaling", fNewScale).m_Result.Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
        else
        {
          for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            ezVec3 vNewScale = obj.m_vLocalScaling.CompMul(vScale);

            if (pAccessor->SetValue(obj.m_pObject, "LocalScaling", vNewScale).m_Result.Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<ezOrthoGizmoContext>())
      {
        const ezOrthoGizmoContext* pOrtho = static_cast<const ezOrthoGizmoContext*>(e.m_pGizmo);

        const float fScale = pOrtho->GetScalingResult();
        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];
          const float fNewScale = obj.m_fLocalUniformScaling * fScale;

          if (pAccessor->SetValue(obj.m_pObject, "LocalUniformScaling", fNewScale).m_Result.Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (bCancel)
        pAccessor->CancelTransaction();
      else
        pAccessor->FinishTransaction();
    }
    break;
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragToPositionGizmoEditTool, 1, ezRTTIDefaultAllocator<ezDragToPositionGizmoEditTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDragToPositionGizmoEditTool::ezDragToPositionGizmoEditTool()
{

  m_DragToPosGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

ezDragToPositionGizmoEditTool::~ezDragToPositionGizmoEditTool()
{
  m_DragToPosGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void ezDragToPositionGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_DragToPosGizmo.SetOwner(GetWindow(), nullptr);
}

void ezDragToPositionGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_DragToPosGizmo.SetVisible(visible);
}

void ezDragToPositionGizmoEditTool::ApplyGizmoTransformation(const ezTransform& transform)
{
  m_DragToPosGizmo.SetTransformation(transform);
}

void ezDragToPositionGizmoEditTool::TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e)
{
  ezObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
          QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when shift is held while dragging the item
      if (e.m_pGizmo == &m_DragToPosGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case ezGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      ezTransform tNew;

      if (e.m_pGizmo == &m_DragToPosGizmo)
      {
        const ezVec3 vTranslate = m_DragToPosGizmo.GetTranslationResult();
        const ezQuat qRot = m_DragToPosGizmo.GetRotationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (m_DragToPosGizmo.ModifiesRotation())
          {
            tNew.m_qRotation = qRot;
          }

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew,
                                                    TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation | TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;
  }
}
