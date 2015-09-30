#pragma once

#include <EditorFramework/Plugin.h>
#include <QWidget>
#include <Foundation/Containers/HybridArray.h>
#include <CoreUtils/Graphics/Camera.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezDocumentWindow3D;
class ezEditorInputContext;

class EZ_EDITORFRAMEWORK_DLL ezEngineViewWidget : public QWidget
{
  Q_OBJECT

public:
  ezEngineViewWidget(QWidget* pParent, ezDocumentWindow3D* pDocumentWindow, ezSceneViewConfig* pViewConfig);
  ~ezEngineViewWidget();

  /// \brief Add input contexts in the order in which they are supposed to be processed
  ezHybridArray<ezEditorInputContext*, 8> m_InputContexts;

  virtual void paintEvent(QPaintEvent* event) override;
  virtual QPaintEngine* paintEngine() const override { return nullptr; }

  /// \brief Used to deactivate shortcuts
  virtual bool eventFilter(QObject* object, QEvent* event) override;

  /// \brief Returns the ID of this view
  ezUInt32 GetViewID() const { return m_uiViewID; }
  ezDocumentWindow3D* GetDocumentWindow() const { return m_pDocumentWindow; }
  virtual void SyncToEngine();

  ezSceneViewConfig* m_pViewConfig;

protected:
  virtual void resizeEvent(QResizeEvent* event) override;

  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void focusOutEvent(QFocusEvent* e) override;

  ezUInt32 m_uiViewID;
  ezDocumentWindow3D* m_pDocumentWindow;

  static ezUInt32 s_uiNextViewID;
};


