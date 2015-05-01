#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_ImageWidget.h>

class EZ_EDITORFRAMEWORK_DLL QtImageWidget : public QWidget, public Ui_ImageWidget
{
  Q_OBJECT

public:
  QtImageWidget(QWidget* parent);
  ~QtImageWidget();

  void SetImage(QPixmap pixmap);

  void SetImageSize(float fScale = 1.0f);
  void ScaleImage(float fFactor);

private slots:

  void on_ButtonZoomIn_clicked();
  void on_ButtonZoomOut_clicked();
  void on_ButtonResetZoom_clicked();

private:

  void ImageApplyScale();

  float m_fCurrentScale;

};