#include <PCH.h>
#include <EditorFramework/Widgets/ImageWidget.moc.h>
#include <Foundation/Math/Math.h>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>


QtImageWidget::QtImageWidget(QWidget* parent, bool bShowButtons) : QWidget(parent)
{
  setupUi(this);

  m_fCurrentScale = 1.0f;
  ScrollArea->setWidgetResizable(false);
  LabelImage->setScaledContents(false);

  if (!bShowButtons)
    ButtonBar->setVisible(false);
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
  float fPrevScale = m_fCurrentScale;
  m_fCurrentScale = ezMath::Clamp(m_fCurrentScale * fFactor, 0.2f, 5.0f);

  fFactor = m_fCurrentScale / fPrevScale;

  ImageApplyScale();
  
  ScrollArea->horizontalScrollBar()->setValue(int(fFactor * ScrollArea->horizontalScrollBar()->value() + ((fFactor - 1) * ScrollArea->horizontalScrollBar()->pageStep() / 2)));
  ScrollArea->verticalScrollBar()->setValue(int(fFactor * ScrollArea->verticalScrollBar()->value() + ((fFactor - 1) * ScrollArea->verticalScrollBar()->pageStep() / 2)));
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
