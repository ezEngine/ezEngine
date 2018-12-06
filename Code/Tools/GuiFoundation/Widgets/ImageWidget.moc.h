#pragma once

#include <GuiFoundation/Basics.h>
#include <Code/Tools/GuiFoundation/ui_ImageWidget.h>
#include <QGraphicsScene>

class QGraphicsPixmapItem;

class EZ_GUIFOUNDATION_DLL ezQtImageScene : public QGraphicsScene
{
public:
  ezQtImageScene(QObject* pParent = nullptr);

  void SetImage(QPixmap pixmap);

private:
  QPixmap m_pixmap;
  QGraphicsPixmapItem* m_pImageItem;
};

class EZ_GUIFOUNDATION_DLL ezQtImageWidget : public QWidget, public Ui_ImageWidget
{
  Q_OBJECT

public:
  ezQtImageWidget(QWidget* parent, bool bShowButtons = true);
  ~ezQtImageWidget();

  void SetImage(QPixmap pixmap);

  void SetImageSize(float fScale = 1.0f);
  void ScaleImage(float fFactor);

private slots:

  void on_ButtonZoomIn_clicked();
  void on_ButtonZoomOut_clicked();
  void on_ButtonResetZoom_clicked();

private:

  void ImageApplyScale();

  ezQtImageScene* m_pScene;
  float m_fCurrentScale;

};
