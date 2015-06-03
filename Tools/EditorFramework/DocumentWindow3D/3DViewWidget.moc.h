#pragma once

#include <EditorFramework/Plugin.h>
#include <QWidget>
#include <Foundation/Containers/HybridArray.h>

class ezDocumentWindow3D;
class ezEditorInputContext;

class EZ_EDITORFRAMEWORK_DLL ez3DViewWidget : public QWidget
{
public:
  ez3DViewWidget(QWidget* pParent, ezDocumentWindow3D* pDocument);

  /// \brief Add input contexts in the order in which they are supposed to be processed
  ezHybridArray<ezEditorInputContext*, 8> m_InputContexts;

  virtual void paintEvent(QPaintEvent* event) override;

protected:
  virtual void resizeEvent(QResizeEvent* event) override;

  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void focusOutEvent(QFocusEvent* e) override;

  ezDocumentWindow3D* m_pDocumentWindow;
};


