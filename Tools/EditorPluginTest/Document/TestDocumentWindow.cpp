#include <PCH.h>
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

ezTestDocumentWindow::ezTestDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_pCenterWidget = new ez3DViewWidget(this, this);
  m_pCenterWidget->m_MoveContext.m_pCamera = &m_Camera;
  
  m_pCenterWidget->setAutoFillBackground(false);
  setCentralWidget(m_pCenterWidget);

  m_pRestartButtonLayout = new QHBoxLayout(this);
  m_pRestartButtonLayout->setMargin(0);

  centralWidget()->setLayout(m_pRestartButtonLayout);

  m_pRestartButton = new QPushButton(centralWidget());
  m_pRestartButton->setText("Restart Engine View Process");
  m_pRestartButton->setVisible(ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed());
  m_pRestartButton->setMaximumWidth(200);
  m_pRestartButton->setMinimumHeight(50);
  m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezTestDocumentWindow::SlotRestartEngineProcess);

  m_pRestartButtonLayout->addWidget(m_pRestartButton);

  SetTargetFramerate(24);

  m_pEngineView = ezEditorEngineProcessConnection::GetInstance()->CreateEngineConnection(pDocument);
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));

  GetDocument()->GetObjectTree()->m_StructureEvents.AddEventHandler(ezDelegate<void (const ezDocumentObjectTreeStructureEvent&)>(&ezTestDocumentWindow::DocumentTreeEventHandler, this));
  GetDocument()->GetObjectTree()->m_PropertyEvents.AddEventHandler(ezDelegate<void (const ezDocumentObjectTreePropertyEvent&)>(&ezTestDocumentWindow::PropertyEventHandler, this));

  m_pEngineView->SendDocument();
  
  m_Camera.SetCameraMode(ezCamera::CameraMode::PerspectiveFixedFovY, 80.0f, 0.1f, 1000.0f);
  //m_Camera.LookAt(ezVec3::ZeroVector(), ezVec3(0, 0, -1), ezVec3(0, 1, 0));
  m_Camera.LookAt(ezVec3(0.5f, 1.5f, 2.0f), ezVec3(0.0f, 0.5f, 0.0f), ezVec3(0.0f, 1.0f, 0.0f));
}

ezTestDocumentWindow::~ezTestDocumentWindow()
{
  GetDocument()->GetObjectTree()->m_PropertyEvents.RemoveEventHandler(ezDelegate<void (const ezDocumentObjectTreePropertyEvent&)>(&ezTestDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectTree()->m_StructureEvents.RemoveEventHandler(ezDelegate<void (const ezDocumentObjectTreeStructureEvent&)>(&ezTestDocumentWindow::DocumentTreeEventHandler, this));
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezDelegate<void (const ezEditorEngineProcessConnection::Event&)>(&ezTestDocumentWindow::EngineViewProcessEventHandler, this));
  ezEditorEngineProcessConnection::GetInstance()->DestroyEngineConnection(m_pEngineView);
}

void ezTestDocumentWindow::PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e)
{
  m_pEngineView->SendObjectProperties(e);
}

void ezTestDocumentWindow::DocumentTreeEventHandler(const ezDocumentObjectTreeStructureEvent& e)
{
  m_pEngineView->SendDocumentTreeChange(e);
}

void ezTestDocumentWindow::EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      m_pRestartButton->setVisible(true);
      m_pRestartButton->update();
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      m_pRestartButton->setVisible(false);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    break;
  }
}

void ezTestDocumentWindow::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetInstance()->RestartProcess();

  m_pEngineView->SendDocument();
}

void ezTestDocumentWindow::InternalRedraw()
{
  m_pCenterWidget->m_MoveContext.Update();
  SendRedrawMsg();
}

void ezTestDocumentWindow::SendRedrawMsg()
{
  ezEngineViewCameraMsg cam;
  cam.m_fNearPlane = m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_Camera.GetFarPlane();
  cam.m_iCameraMode = (ezInt8) m_Camera.GetCameraMode();
  cam.m_fFovOrDim = m_Camera.GetFovOrDim();
  cam.m_vDirForwards = m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_Camera.GetCenterPosition();
  m_Camera.GetViewMatrix(cam.m_ViewMatrix);
  m_Camera.GetProjectionMatrix((float) m_pCenterWidget->width() / (float) m_pCenterWidget->height(), ezProjectionDepthRange::ZeroToOne, cam.m_ProjMatrix);

  m_pEngineView->SendMessage(&cam);

  ezEngineViewRedrawMsg msg;
  msg.m_uiHWND = (ezUInt32) m_pCenterWidget->winId();
  msg.m_uiWindowWidth = m_pCenterWidget->width();
  msg.m_uiWindowHeight = m_pCenterWidget->height();

  m_pEngineView->SendMessage(&msg);
}

void ezTestDocumentWindow::keyPressEvent(QKeyEvent* e)
{
  if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
  {
    if (e->key() == Qt::Key_Z)
    {
      if (GetDocument()->GetCommandHistory()->CanUndo())
      {
        GetDocument()->GetCommandHistory()->Undo();
        return;
      }
    }
    else if (e->key() == Qt::Key_Y)
    {
      if (GetDocument()->GetCommandHistory()->CanRedo())
      {
        GetDocument()->GetCommandHistory()->Redo();
        return;
      }
    }
  }

  ezDocumentWindow::keyPressEvent(e);
}

ezCameraMoveContext::ezCameraMoveContext()
{
  m_pCamera = nullptr;
  m_fMoveSpeed = 1.0f;

  m_bRun = false;
  m_bSlowDown = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;

  m_LastUpdate = ezTime::Now();
}

void ezCameraMoveContext::Update()
{
  ezTime diff = ezTime::Now() - m_LastUpdate;
  m_LastUpdate = ezTime::Now();

  const double TimeDiff = ezMath::Min(diff.GetSeconds(), 0.1);

  float fSpeedFactor = m_fMoveSpeed * TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;
  if (m_bSlowDown)
    fSpeedFactor *= 0.2f;

  if (m_bMoveForwards)
    m_pCamera->MoveLocally(ezVec3(0, 0, -1) * fSpeedFactor);
  if (m_bMoveBackwards)
    m_pCamera->MoveLocally(ezVec3(0, 0, 1) * fSpeedFactor);
  if (m_bMoveRight)
    m_pCamera->MoveLocally(ezVec3(1, 0, 0) * fSpeedFactor);
  if (m_bMoveLeft)
    m_pCamera->MoveLocally(ezVec3(-1, 0, 0) * fSpeedFactor);
  if (m_bMoveUp)
    m_pCamera->MoveGlobally(ezVec3(0, 1, 0) * fSpeedFactor);
  if (m_bMoveDown)
    m_pCamera->MoveGlobally(ezVec3(0, -1, 0) * fSpeedFactor);
}

ez3DViewWidget::ez3DViewWidget(QWidget* pParent, ezDocumentWindow* pDocument) : QWidget(pParent), m_pDocument(pDocument)
{
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
}

void ez3DViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  if (m_MoveContext.keyReleaseEvent(e))
    return;

  QWidget::keyReleaseEvent(e);
}

void ez3DViewWidget::keyPressEvent(QKeyEvent* e)
{
  if (m_MoveContext.keyPressEvent(e))
    return;

  QWidget::keyPressEvent(e);
}

void ez3DViewWidget::mousePressEvent(QMouseEvent* e)
{
  if (m_MoveContext.mousePressEvent(e))
    return;

  QWidget::mousePressEvent(e);
}

void ez3DViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  if (m_MoveContext.mouseReleaseEvent(e))
    return;

  QWidget::mouseReleaseEvent(e);
}

void ez3DViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  if (m_MoveContext.mouseMoveEvent(e))
    return;

  QWidget::mouseMoveEvent(e);
}


ezQtCameraMoveContext::ezQtCameraMoveContext()
{
  m_bRotateCamera = false;
}

bool ezQtCameraMoveContext::keyReleaseEvent(QKeyEvent* e)
{
  if (m_pCamera == nullptr)
    return false;

  m_bRun = (e->modifiers() == Qt::KeyboardModifier::ShiftModifier);
  m_bSlowDown = (e->modifiers() == Qt::KeyboardModifier::AltModifier);

  switch (e->key())
  {
  case Qt::Key_W:
    m_bMoveForwards = false;
    return true;
  case Qt::Key_S:
    m_bMoveBackwards = false;
    return true;
  case Qt::Key_A:
    m_bMoveLeft = false;
    return true;
  case Qt::Key_D:
    m_bMoveRight = false;
    return true;
  case Qt::Key_Q:
    m_bMoveUp = false;
    return true;
  case Qt::Key_E:
    m_bMoveDown = false;
    return true;
  }

  return false;
}

bool ezQtCameraMoveContext::keyPressEvent(QKeyEvent* e)
{
  if (m_pCamera == nullptr)
    return false;

  if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
    return false;

  m_bRun = (e->modifiers() == Qt::KeyboardModifier::ShiftModifier);
  m_bSlowDown = (e->modifiers() == Qt::KeyboardModifier::AltModifier);

  switch (e->key())
  {
  case Qt::Key_W:
    m_bMoveForwards = true;
    return true;
  case Qt::Key_S:
    m_bMoveBackwards = true;
    return true;
  case Qt::Key_A:
    m_bMoveLeft = true;
    return true;
  case Qt::Key_D:
    m_bMoveRight = true;
    return true;
  case Qt::Key_Q:
    m_bMoveUp = true;
    return true;
  case Qt::Key_E:
    m_bMoveDown = true;
    return true;
  }

  return false;
}

bool ezQtCameraMoveContext::mousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return false;

  if (e->modifiers() != Qt::KeyboardModifier::NoModifier)
    return false;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bRotateCamera = true;
    m_LastMousePos = e->localPos();
    return true;
  }

  return false;
}

bool ezQtCameraMoveContext::mouseReleaseEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return false;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bRotateCamera = false;
    return true;
  }

  return false;
}

bool ezQtCameraMoveContext::mouseMoveEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return false;

  if (m_bRotateCamera)
  {
    const QPointF diff = e->localPos() - m_LastMousePos;

    float fRotateX = diff.y() * -0.5f;
    float fRotateY = diff.x() * -0.5f;

    m_pCamera->RotateGlobally(ezAngle::Radian(0), ezAngle::Degree(fRotateY), ezAngle::Radian(0));
    m_pCamera->RotateLocally(ezAngle::Degree(fRotateX), ezAngle::Radian(0), ezAngle::Radian(0));

    m_LastMousePos = e->localPos();
  }

  return false;
}


