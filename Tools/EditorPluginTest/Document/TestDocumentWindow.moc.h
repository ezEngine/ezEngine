#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <System/Window/Window.h>
#include <QProcess>
#include <QSharedMemory>
#include <CoreUtils/Graphics/Camera.h>
#include <QPaintEvent>

class QWidget;
class QHBoxLayout;
class QPushButton;

class ezCameraMoveContext
{
public:
  ezCameraMoveContext();
  void Update();
  virtual void Reset();
  virtual void LoadState() { }

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

private:
  ezTime m_LastUpdate;
};

class ezQtCameraMoveContext : public ezCameraMoveContext
{
public:
  ezQtCameraMoveContext(QWidget* pParentWidget, ezDocumentBase* pDocument);

  virtual void Reset() override;

  virtual void LoadState() override;

  bool keyPressEvent(QKeyEvent* e);
  bool keyReleaseEvent(QKeyEvent* e);
  bool mousePressEvent(QMouseEvent* e);
  bool mouseReleaseEvent(QMouseEvent* e);
  bool mouseMoveEvent(QMouseEvent* e);
  bool wheelEvent(QWheelEvent* e);

protected:
  ezDocumentBase* m_pDocument;

private:
  void SetMoveSpeed(ezInt32 iSpeed);
  void ResetCursor();
  void SetBlankCursor();
  void SetCursorToWindowCenter();

  QWidget* m_pParentWidget;
  QPoint m_LastMousePos;
  QPoint m_OriginalMousePos;
  bool m_bTempMousePosition;

  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
  ezInt32 m_iMoveSpeed;
};

class ez3DViewWidget : public QWidget
{
public:
  ez3DViewWidget(QWidget* pParent, ezDocumentWindow* pDocument);

  ezQtCameraMoveContext m_MoveContext;

  virtual void paintEvent(QPaintEvent* event) override { event->accept(); }

protected:
  virtual void resizeEvent(QResizeEvent* event) override
  {
    m_pDocument->TriggerRedraw();
  }

  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void focusOutEvent(QFocusEvent* e) override;
  
  ezDocumentWindow* m_pDocument;
};

class ezTestDocumentWindow : public ezDocumentWindow3D
{
  Q_OBJECT

public:
  ezTestDocumentWindow(ezDocumentBase* pDocument);
  ~ezTestDocumentWindow();

  virtual const char* GetGroupName() const { return "Scene"; }

protected:
  void keyPressEvent(QKeyEvent* e) override;

private slots:
  

private:
  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  virtual void InternalRedraw() override;
  void DocumentTreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);
  void PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e);

  void SendRedrawMsg();

  ez3DViewWidget* m_pCenterWidget;
  
  ezCamera m_Camera;
};