#pragma once

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Size.h>
#include <QWidget>

class ezQtEngineDocumentWindow;
class ezEditorInputContext;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;
class ezViewMarqueePickingResultMsgToEditor;

struct EZ_EDITORFRAMEWORK_DLL ezObjectPickingResult
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
  ezQtEngineViewWidget(QWidget* pParent, ezQtEngineDocumentWindow* pDocumentWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtEngineViewWidget();

  /// \brief Add input contexts in the order in which they are supposed to be processed
  ezHybridArray<ezEditorInputContext*, 8> m_InputContexts;

  /// \brief Returns the ID of this view
  ezUInt32 GetViewID() const { return m_uiViewID; }
  ezQtEngineDocumentWindow* GetDocumentWindow() const { return m_pDocumentWindow; }

  /// \brief Sends the redraw message to the engine
  virtual void SyncToEngine();

  void GetCameraMatrices(ezMat4& out_ViewMatrix, ezMat4& out_ProjectionMatrix) const;

  ezEngineViewConfig* m_pViewConfig;

  /// \brief Called every frame to move the camera to its current target (focus on selection, etc.)
  void UpdateCameraInterpolation();

  /// \brief The view's camera will be interpolated to the given coordinates
  void InterpolateCameraTo(const ezVec3& vPosition, const ezVec3& vDirection, float fFovOrDim, const ezVec3* pNewUpDirection = nullptr, bool bImmediate = false);

  /// \brief If disabled, no picking takes place in this view.
  ///
  /// Disabled in views that do not need picking (material asset, particle asset, etc.)
  /// and when the mouse is outside a view, to prevent useless picking.
  void SetEnablePicking(bool bEnable) { m_bUpdatePickingData = bEnable; }

  /// \brief Disabled during drag&drop operations, to prevent picking against the dragged object.
  virtual bool IsPickingAgainstSelectionAllowed() const { return !m_bInDragAndDropOperation; }

  /// \brief Holds information about the viewport that the user just now hovered over and what object was picked last
  struct InteractionContext
  {
    ezQtEngineViewWidget* m_pLastHoveredViewWidget = nullptr;
    const ezObjectPickingResult* m_pLastPickingResult = nullptr;
  };

  /// \brief Returns the latest information about what viewport the user interacted with.
  static const InteractionContext& GetInteractionContext() { return s_InteractionContext; }

  /// \brief Overrides the InteractionContext with custom values. Mostly useful for injecting procedural user interaction for unit tests.
  static void SetInteractionContext(const InteractionContext& ctxt) { s_InteractionContext = ctxt; }

  /// \brief Supposed to open a context menu at the given position. Derived classes must implement OnOpenContextMenu and do the actual work there.
  void OpenContextMenu(QPoint globalPos);

  /// \brief Starts a picking operation for the given pixel position in this view. Returns the most recent picking information in the meantime.
  const ezObjectPickingResult& PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const;

  /// \brief Similar to PickObject, but computes the intersection with the given plane instead.
  ezResult PickPlane(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY, const ezPlane& plane, ezVec3& out_Position) const;

  /// \brief Processes incoming messages from the engine that are meant for this particular view. Mostly picking results.
  void HandleViewMessage(const ezEditorEngineViewMsg* pMsg);

  /// \brief Returns a plane that can be used for picking, when nothing else is available
  /// Orthographic views would typically return their projection planes, perspective views may return the ground plane
  virtual ezPlane GetFallbackPickingPlane(ezVec3 vPointOnPlane = ezVec3(0)) const;

  /// If this is set to a non-zero value, all rendering will use a fixed resolution, instead of the actual window size.
  /// This is useful for unit tests, to guarantee a specific output size, to be able to do image comparisons.
  static ezSizeU32 s_FixedResolution;

  void TakeScreenshot(const char* szOutputPath) const;

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
  virtual void HandleMarqueePickingResult(const ezViewMarqueePickingResultMsgToEditor* pMsg) {}

private Q_SLOTS:
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
