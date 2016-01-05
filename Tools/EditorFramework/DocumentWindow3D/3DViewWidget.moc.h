#pragma once

#include <EditorFramework/Plugin.h>
#include <QWidget>
#include <Foundation/Containers/HybridArray.h>
#include <CoreUtils/Graphics/Camera.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

class ezQtEngineDocumentWindow;
class ezEditorInputContext;
class QHBoxLayout;
class QPushButton;

class EZ_EDITORFRAMEWORK_DLL ezQtEngineViewWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtEngineViewWidget(QWidget* pParent, ezQtEngineDocumentWindow* pDocumentWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtEngineViewWidget();

  /// \brief Add input contexts in the order in which they are supposed to be processed
  ezHybridArray<ezEditorInputContext*, 8> m_InputContexts;

  virtual void paintEvent(QPaintEvent* event) override;
  virtual QPaintEngine* paintEngine() const override { return nullptr; }

  /// \brief Used to deactivate shortcuts
  virtual bool eventFilter(QObject* object, QEvent* event) override;

  /// \brief Returns the ID of this view
  ezUInt32 GetViewID() const { return m_uiViewID; }
  ezQtEngineDocumentWindow* GetDocumentWindow() const { return m_pDocumentWindow; }
  virtual void SyncToEngine();

  ezSceneViewConfig* m_pViewConfig;

  void UpdateCameraInterpolation();

  /// \brief The view's camera will be interpolated to the given coordinates
  void InterpolateCameraTo(const ezVec3& vPosition, const ezVec3& vDirection, float fFovOrDim);

  void SetEnablePicking(bool bEnable) { m_bUpdatePickingData = bEnable; }

private slots:
  void SlotRestartEngineProcess();

protected:
  virtual void resizeEvent(QResizeEvent* event) override;

  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void focusOutEvent(QFocusEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;


  void EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);
  void ShowRestartButton(bool bShow);

protected:
  bool m_bUpdatePickingData;
  bool m_bInDragAndDropOperation;
  ezUInt32 m_uiViewID;
  ezQtEngineDocumentWindow* m_pDocumentWindow;

  static ezUInt32 s_uiNextViewID;

  // Camera Interpolation
  float m_fCameraLerp;
  float m_fCameraStartFovOrDim;
  float m_fCameraTargetFovOrDim;
  ezVec3 m_vCameraStartPosition;
  ezVec3 m_vCameraTargetPosition;
  ezVec3 m_vCameraStartDirection;
  ezVec3 m_vCameraTargetDirection;
  ezTime m_LastCameraUpdate;

  QHBoxLayout* m_pRestartButtonLayout;
  QPushButton* m_pRestartButton;
};


