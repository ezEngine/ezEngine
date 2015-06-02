#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <System/Window/Window.h>
#include <QProcess>
#include <QSharedMemory>
#include <CoreUtils/Graphics/Camera.h>
#include <QPaintEvent>

class QWidget;
class QHBoxLayout;
class QPushButton;

class ezDocumentWindow3D;

class ezEditorInputContext
{
public:
  ezEditorInputContext(QWidget* pParentWidget, ezDocumentBase* pDocument, ezDocumentWindow3D* pDocumentWindow) 
  {
    m_pDocument = pDocument;
    m_pParentWidget = pParentWidget;
    m_pDocumentWindow = pDocumentWindow;
  }

  virtual ~ezEditorInputContext() {}

  virtual void FocusLost() {}

  virtual bool keyPressEvent(QKeyEvent* e) { return false; }
  virtual bool keyReleaseEvent(QKeyEvent* e) { return false; }
  virtual bool mousePressEvent(QMouseEvent* e) { return false; }
  virtual bool mouseReleaseEvent(QMouseEvent* e) { return false; }
  virtual bool mouseMoveEvent(QMouseEvent* e) { return false; }
  virtual bool wheelEvent(QWheelEvent* e) { return false; }

  static void SetActiveInputContext(ezEditorInputContext* pContext) { s_pActiveInputContext = pContext; }

  void MakeActiveInputContext(bool bActive = true)
  {
    if (bActive)
      s_pActiveInputContext = this;
    else
      s_pActiveInputContext = nullptr;
  }

  static bool IsAnyInputContextActive() { return s_pActiveInputContext != nullptr; }

  static ezEditorInputContext* GetActiveInputContext() { return s_pActiveInputContext; }

  static void UpdateActiveInputContext()
  {
    if (s_pActiveInputContext != nullptr)
      s_pActiveInputContext->UpdateContext();
  }

  bool IsActiveInputContext() const
  {
    return s_pActiveInputContext == this;
  }

protected:
  ezDocumentBase* m_pDocument;
  ezDocumentWindow3D* m_pDocumentWindow;
  QWidget* m_pParentWidget;

private:
  static ezEditorInputContext* s_pActiveInputContext;

  virtual void UpdateContext() {}
};

class ezSelectionContext : public ezEditorInputContext
{
public:
  ezSelectionContext(QWidget* pParentWidget, ezDocumentBase* pDocument, ezDocumentWindow3D* pDocumentWindow);

  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;
};

class ezCameraMoveContext : public ezEditorInputContext
{
public:
  ezCameraMoveContext(QWidget* pParentWidget, ezDocumentBase* pDocument, ezDocumentWindow3D* pDocumentWindow);


  virtual void FocusLost() override;

  void LoadState();

  virtual bool keyPressEvent(QKeyEvent* e) override;
  virtual bool keyReleaseEvent(QKeyEvent* e) override;
  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;
  virtual bool wheelEvent(QWheelEvent* e) override;

  void SetCamera(ezCamera* pCamera) { m_pCamera = pCamera; }


private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(ezInt32 iSpeed);
  void ResetCursor();
  void SetBlankCursor();
  void SetCursorToWindowCenter();

  QPoint m_LastMousePos;
  QPoint m_OriginalMousePos;
  bool m_bTempMousePosition;

  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
  ezInt32 m_iMoveSpeed;

  ezCamera* m_pCamera;
  float m_fMoveSpeed;

  bool m_bRun;
  bool m_bSlowDown;
  bool m_bMoveForwards;
  bool m_bMoveBackwards;
  bool m_bMoveRight;
  bool m_bMoveLeft;
  bool m_bMoveUp;
  bool m_bMoveDown;
  bool m_bMoveForwardsInPlane;
  bool m_bMoveBackwardsInPlane;
  bool m_bDidMoveMouse[3]; // Left Click, Right Click, Middle Click

  ezTime m_LastUpdate;
};

class ez3DViewWidget : public QWidget
{
public:
  ez3DViewWidget(QWidget* pParent, ezDocumentWindow3D* pDocument);

  ezSelectionContext m_SelectionContext;
  ezCameraMoveContext m_MoveContext;

  virtual void paintEvent(QPaintEvent* event) override { event->accept(); }

protected:
  virtual void resizeEvent(QResizeEvent* event) override
  {
    m_pDocumentWindow->TriggerRedraw();
  }

  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void focusOutEvent(QFocusEvent* e) override;

  ezDocumentWindow3D* m_pDocumentWindow;
};

class ezTestDocumentWindow : public ezDocumentWindow3D
{
  Q_OBJECT

public:
  ezTestDocumentWindow(ezDocumentBase* pDocument);
  ~ezTestDocumentWindow();

  virtual const char* GetGroupName() const { return "Scene"; }

  private slots:


private:
  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  virtual void InternalRedraw() override;
  void DocumentTreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);
  void PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e);

  void SendRedrawMsg();

  ez3DViewWidget* m_pCenterWidget;

  ezCamera m_Camera;

  /// \todo Broken delegates
  ezDelegate<void(const ezDocumentObjectTreePropertyEvent&)> m_DelegatePropertyEvents;
  ezDelegate<void(const ezDocumentObjectTreeStructureEvent&)> m_DelegateDocumentTreeEvents;
};