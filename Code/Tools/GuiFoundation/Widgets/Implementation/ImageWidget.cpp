#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <Foundation/Math/Math.h>
#include <QScrollArea>
#include <QScrollBar>
#include <QGraphicsPixmapItem>

QtImageScene::QtImageScene(QObject* pParent) : QGraphicsScene(pParent)
{
  m_pImageItem = nullptr;
  setItemIndexMethod(QGraphicsScene::NoIndex);
}

void QtImageScene::SetImage(QPixmap pixmap)
{
  if (m_pImageItem)
    delete m_pImageItem;

  m_pixmap = pixmap;
  m_pImageItem = addPixmap(m_pixmap);
  setSceneRect(0, 0, m_pixmap.width(), m_pixmap.height());
}



QtImageWidget::QtImageWidget(QWidget* parent, bool bShowButtons) : QWidget(parent)
{
  setupUi(this);
  m_pScene = new QtImageScene(GraphicsView);
  GraphicsView->setScene(m_pScene);

  m_fCurrentScale = 1.0f;

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
}

void QtImageWidget::ImageApplyScale()
{
  QTransform scale = QTransform::fromScale(m_fCurrentScale, m_fCurrentScale);
  GraphicsView->setTransform(scale);
}

void QtImageWidget::SetImage(QPixmap pixmap)
{
  m_pScene->SetImage(pixmap);
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
