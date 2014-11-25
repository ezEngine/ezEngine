#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Pipeline/RenderHelper.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <System/Window/Window.h>
#include <QProcess>
#include <QSharedMemory>
#include <CoreUtils/Graphics/Camera.h>

class QWidget;
class QHBoxLayout;
class QPushButton;

class ezCameraMoveContext
{
public:
  ezCameraMoveContext();
  void Update();
  virtual void Reset();

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
  ezQtCameraMoveContext();

  virtual void Reset() override;

  bool keyPressEvent(QKeyEvent* e);
  bool keyReleaseEvent(QKeyEvent* e);
  bool mousePressEvent(QMouseEvent* e);
  bool mouseReleaseEvent(QMouseEvent* e);
  bool mouseMoveEvent(QMouseEvent* e);

private:
  QPointF m_LastMousePos;
  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
};

class ez3DViewWidget : public QWidget
{
public:
  ez3DViewWidget(QWidget* pParent, ezDocumentWindow* pDocument);

  ezQtCameraMoveContext m_MoveContext;

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
  virtual void focusOutEvent(QFocusEvent* e) override;
  
  ezDocumentWindow* m_pDocument;
};

class ezTestDocumentWindow : public ezDocumentWindow
{
  Q_OBJECT

public:
  ezTestDocumentWindow(ezDocumentBase* pDocument);
  ~ezTestDocumentWindow();

protected:
  void keyPressEvent(QKeyEvent* e) override;

private slots:
  void SlotRestartEngineProcess();

private:
  virtual void InternalRedraw() override;
  void EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);
  void DocumentTreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);
  void PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e);

  void SendRedrawMsg();

  ez3DViewWidget* m_pCenterWidget;
  ezEditorEngineConnection* m_pEngineView;
  QHBoxLayout* m_pRestartButtonLayout;
  QPushButton* m_pRestartButton;
  ezCamera m_Camera;
};