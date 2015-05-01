#include <PCH.h>
#include <EditorFramework/Widgets/ImageWidget.moc.h>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>


QtImageWidget::QtImageWidget(QWidget* parent) : QWidget(parent)
{
  setupUi(this);

  m_fCurrentScale = 1.0f;
  ScrollArea->setWidgetResizable(false);
  LabelImage->setScaledContents(false);
}

QtImageWidget::~QtImageWidget()
{

}

void QtImageWidget::SetImageSize(float fScale)
{
  if (m_fCurrentScale == fScale)
    return;

  m_fCurrentScale = fScale;
  ImageApplyScale();
}

void QtImageWidget::ScaleImage(float fFactor)
{
  m_fCurrentScale *= fFactor;
  ImageApplyScale();
  
  //ScrollImage->horizontalScrollBar()->setValue(int(fFactor * ScrollImage->horizontalScrollBar()->value() + ((fFactor - 1) * ScrollImage->horizontalScrollBar()->pageStep() / 2)));
  //ScrollImage->verticalScrollBar()->setValue(int(fFactor * ScrollImage->verticalScrollBar()->value() + ((fFactor - 1) * ScrollImage->verticalScrollBar()->pageStep() / 2)));
}

void QtImageWidget::ImageApplyScale()
{
  if (m_fCurrentScale == 1.0f)
  {
    LabelImage->setScaledContents(false);
    LabelImage->adjustSize();
  }
  else
  {
    LabelImage->setScaledContents(true);
    QSize s = m_fCurrentScale * LabelImage->pixmap()->size();
    LabelImage->resize(s);
  }

  QSize s = LabelImage->size();
  //LabelImage->updateGeometry();
  //LabelImage->update();
  int i = 0;
}

void QtImageWidget::SetImage(QPixmap pixmap)
{
  LabelImage->setPixmap(pixmap);
  ImageApplyScale();
}

void QtImageWidget::on_ButtonZoomIn_clicked()
{
  ScaleImage(1.25f);
}

void QtImageWidget::on_ButtonZoomOut_clicked()
{
  ScaleImage(0.75f);
}

void QtImageWidget::on_ButtonResetZoom_clicked()
{
  SetImageSize(1.0f);
}
