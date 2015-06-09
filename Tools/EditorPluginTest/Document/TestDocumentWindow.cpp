#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTest/Document/TestDocumentWindow.moc.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <Foundation/IO/OSFile.h>
#include <QTimer>
#include <QPushButton>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <qlayout.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Core/World/GameObject.h>
#include <QKeyEvent>
#include <Foundation/Time/Time.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>

ezTestDocumentWindow::ezTestDocumentWindow(ezDocumentBase* pDocument) 
  : ezDocumentWindow3D(pDocument)
{
  m_pCenterWidget = new ez3DViewWidget(this, this);
 
  m_pCenterWidget->setAutoFillBackground(false);
  setCentralWidget(m_pCenterWidget);

  SetTargetFramerate(24);

  m_DelegatePropertyEvents = ezMakeDelegate(&ezTestDocumentWindow::PropertyEventHandler, this);
  m_DelegateDocumentTreeEvents = ezMakeDelegate(&ezTestDocumentWindow::DocumentTreeEventHandler, this);

  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(m_DelegateDocumentTreeEvents);
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(m_DelegatePropertyEvents);

  m_Camera.SetCameraMode(ezCamera::CameraMode::PerspectiveFixedFovY, 80.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(0.5f, 1.5f, 2.0f), ezVec3(0.0f, 0.5f, 0.0f), ezVec3(0.0f, 1.0f, 0.0f));

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pDocument, this, &m_Camera);
  m_pMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, m_pCenterWidget, pDocument, this);

  m_pMoveContext->LoadState();
  m_pMoveContext->SetCamera(&m_Camera);

  // add the input contexts in the order in which they are supposed to be processed
  m_pCenterWidget->m_InputContexts.PushBack(m_pSelectionContext);
  m_pCenterWidget->m_InputContexts.PushBack(m_pMoveContext);
  

  {
    // Menu Bar
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "EditorTestDocumentMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "EditorTestDocumentToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("Test Document Window Tool Bar");
    addToolBar(pToolBar);
  }

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezTestDocumentWindow::SelectionManagerEventHandler, this));

  m_TranslateGizmo.SetDocumentWindow3D(this);
  m_RotateGizmo.SetDocumentWindow3D(this);

  m_TranslateGizmo.SetDocumentGuid(pDocument->GetGuid());
  m_TranslateGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezTestDocumentWindow::TransformationGizmoEventHandler, this));

  m_RotateGizmo.SetDocumentGuid(pDocument->GetGuid());
  m_RotateGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezTestDocumentWindow::TransformationGizmoEventHandler, this));
}

ezTestDocumentWindow::~ezTestDocumentWindow()
{
  m_TranslateGizmo.m_BaseEvents.RemoveEventHandler(ezMakeDelegate(&ezTestDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_BaseEvents.RemoveEventHandler(ezMakeDelegate(&ezTestDocumentWindow::TransformationGizmoEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezTestDocumentWindow::SelectionManagerEventHandler, this));

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(m_DelegatePropertyEvents);
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(m_DelegateDocumentTreeEvents);

  EZ_DEFAULT_DELETE(m_pSelectionContext);
  EZ_DEFAULT_DELETE(m_pMoveContext);
}

void ezTestDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  m_pEngineView->SendObjectProperties(e);
}

void ezTestDocumentWindow::DocumentTreeEventHandler(const ezDocumentObjectStructureEvent& e)
{
  m_pEngineView->SendDocumentTreeChange(e);
}

void ezTestDocumentWindow::InternalRedraw()
{
  ezDocumentWindow3D::SyncObjects();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();
}

void ezTestDocumentWindow::SendRedrawMsg()
{
  ezViewCameraMsgToEngine cam;
  cam.m_fNearPlane = m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_Camera.GetFarPlane();
  cam.m_iCameraMode = (ezInt8)m_Camera.GetCameraMode();
  cam.m_fFovOrDim = m_Camera.GetFovOrDim();
  cam.m_vDirForwards = m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_Camera.GetCenterPosition();
  m_Camera.GetViewMatrix(cam.m_ViewMatrix);
  m_Camera.GetProjectionMatrix((float)m_pCenterWidget->width() / (float)m_pCenterWidget->height(), cam.m_ProjMatrix);

  m_pSelectionContext->SetWindowConfig(ezVec2I32(m_pCenterWidget->width(), m_pCenterWidget->height()));

  m_pEngineView->SendMessage(&cam);

  ezViewRedrawMsgToEngine msg;
  msg.m_uiHWND = (ezUInt64)(m_pCenterWidget->winId());
  msg.m_uiWindowWidth = m_pCenterWidget->width();
  msg.m_uiWindowHeight = m_pCenterWidget->height();

  m_pEngineView->SendMessage(&msg);
}

bool ezTestDocumentWindow::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (ezDocumentWindow3D::HandleEngineMessage(pMsg))
    return true;

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezLogMsgToEditor>())
  {
    const ezLogMsgToEditor* pLogMsg = static_cast<const ezLogMsgToEditor*>(pMsg);

    ezLog::Info("Process (%u): '%s'", pLogMsg->m_uiViewID, pLogMsg->m_sText.GetData());

    return true;
  }

  return false;
}


void ezTestDocumentWindow::SelectionManagerEventHandler(const ezSelectionManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManager::Event::Type::SelectionCleared:
    {
      m_TranslateGizmo.SetVisible(false);
      m_RotateGizmo.SetVisible(false);
    }
    break;

  case ezSelectionManager::Event::Type::SelectionSet:
  case ezSelectionManager::Event::Type::ObjectAdded:
    {
      m_TranslateGizmo.SetVisible(true);
      m_RotateGizmo.SetVisible(true);

      if (GetDocument()->GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezGameObject"))
      {
        ezVec3 vPos = GetDocument()->GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetValue("Position").ConvertTo<ezVec3>();
        ezQuat qRot = GetDocument()->GetSelectionManager()->GetSelection()[0]->GetTypeAccessor().GetValue("Rotation").ConvertTo<ezQuat>();
        ezMat4 mt;
        mt.SetTranslationMatrix(vPos);
        mt.SetRotationalPart(qRot.GetAsMat3());

        m_TranslateGizmo.SetTransformation(mt);
        m_RotateGizmo.SetTransformation(mt);
      }
    }
    break;
  }
}

void ezTestDocumentWindow::TransformationGizmoEventHandler(const ezGizmoBase::BaseEvent& e)
{
  switch (e.m_Type)
  {
  case ezGizmoBase::BaseEvent::Type::BeginInteractions:
    {
      GetDocument()->GetCommandHistory()->BeginTemporaryCommands();

    }
    break;

  case ezGizmoBase::BaseEvent::Type::EndInteractions:
    {
      GetDocument()->GetCommandHistory()->EndTemporaryCommands(false);
    }
    break;

  case ezGizmoBase::BaseEvent::Type::Interaction:
    {
      const ezMat4 mTransform = e.m_pGizmo->GetTransformation();

      auto Selection = GetDocument()->GetSelectionManager()->GetSelection();

      GetDocument()->GetCommandHistory()->StartTransaction();

      bool bCancel = false;

      ezSetObjectPropertyCommand cmd;
      cmd.m_bEditorProperty = false;

      if (e.m_pGizmo == &m_TranslateGizmo)
      {
        cmd.m_NewValue = mTransform.GetTranslationVector();
        cmd.SetPropertyPath("Position");
      }

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        ezQuat qRot;
        qRot.SetFromMat3(mTransform.GetRotationalPart());

        cmd.m_NewValue = qRot;
        cmd.SetPropertyPath("Rotation");
      }

      auto hType = ezRTTI::FindTypeByName("ezGameObject");

      for (ezUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
      {
        if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
          continue;

        cmd.m_Object = Selection[sel]->GetGuid();

        ezStatus res = GetDocument()->GetCommandHistory()->AddCommand(cmd);

        //ezUIServices::GetInstance()->MessageBoxStatus(res, "Failed to set the position");

        if (res.m_Result.Failed())
        {
          bCancel = true;
          break;
        }
      }

      GetDocument()->GetCommandHistory()->EndTransaction(bCancel);
    }
    break;
  }

}


