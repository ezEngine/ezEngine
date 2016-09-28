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
class QVBoxLayout;

struct ezObjectPickingResult
{
  ezObjectPickingResult() { Reset(); }
  void Reset();

  ezUuid m_PickedObject;
  ezUuid m_PickedComponent;
  ezUuid m_PickedOther;
  ezUInt32 m_uiPartIndex;
  ezVec3 m_vPickedPosition;
  ezVec3 m_vPickedNormal;
  ezVec3 m_vPickingRayStart;
};

/// \brief Base class for views that show engine output
class EZ_EDITORFRAMEWORK_DLL ezQtEngineViewWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtEngineViewWidget(QWidget* pParent, ezQtEngineDocumentWindow* pDocumentWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtEngineViewWidget();

  /// \brief Add input contexts in the order in which they are supposed to be processed
  ezHybridArray<ezEditorInputContext*, 8> m_InputContexts;

  /// \brief Returns the ID of this view
  ezUInt32 GetViewID() const { return m_uiViewID; }
  ezQtEngineDocumentWindow* GetDocumentWindow() const { return m_pDocumentWindow; }
  virtual void SyncToEngine();

  void GetCameraMatrices(ezMat4& out_ViewMatrix, ezMat4& out_ProjectionMatrix) const;

  ezSceneViewConfig* m_pViewConfig;

  void UpdateCameraInterpolation();

  /// \brief The view's camera will be interpolated to the given coordinates
  void InterpolateCameraTo(const ezVec3& vPosition, const ezVec3& vDirection, float fFovOrDim, const ezVec3* pNewUpDirection = nullptr);

  void SetEnablePicking(bool bEnable) { m_bUpdatePickingData = bEnable; }

  virtual bool IsPickingAgainstSelectionAllowed() const { return !m_bInDragAndDropOperation; }

  /// Context Menu handling

  struct InteractionContext
  {
    InteractionContext();

    ezQtEngineViewWidget* m_pLastHoveredViewWidget;
    const ezObjectPickingResult* m_pLastPickingResult;
  };

  void OpenContextMenu(QPoint globalPos);

  static const InteractionContext& GetInteractionContext() { return s_InteractionContext; }

  const ezObjectPickingResult& PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const;

  /// \brief Processes incoming messages from the engine that are meant for this particular view. Mostly picking results.
  void HandleViewMessage(const ezEditorEngineViewMsg* pMsg);

protected:
  /// \brief Used to deactivate shortcuts
  virtual bool eventFilter(QObject* object, QEvent* event) override;

  virtual void paintEvent(QPaintEvent* event) override;
  virtual QPaintEngine* paintEngine() const override { return nullptr; }

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

protected:
  void EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e);
  void ShowRestartButton(bool bShow);
  virtual void OnOpenContextMenu(QPoint globalPos) {}

private slots:
  void SlotRestartEngineProcess();

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
  ezVec3 m_vCameraUp;
  ezTime m_LastCameraUpdate;

  QHBoxLayout* m_pRestartButtonLayout;
  QPushButton* m_pRestartButton;

  mutable ezObjectPickingResult m_LastPickingResult;

  static InteractionContext s_InteractionContext;
};

/// \brief Wraps and decorates a view widget with a toolbar and layout.
class EZ_EDITORFRAMEWORK_DLL ezQtViewWidgetContainer : public QWidget
{
  Q_OBJECT
public:
  ezQtViewWidgetContainer(QWidget* pParent, ezQtEngineViewWidget* pViewWidget, const char* szToolBarMapping);
  ~ezQtViewWidgetContainer();

  ezQtEngineViewWidget* GetViewWidget() const { return m_pViewWidget; }
  QVBoxLayout* GetLayout() const { return m_pLayout; }
private:
  ezQtEngineViewWidget* m_pViewWidget;
  QVBoxLayout* m_pLayout;
};
