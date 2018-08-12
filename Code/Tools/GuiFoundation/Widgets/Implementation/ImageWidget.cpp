#include <PCH.h>

#include <Foundation/Math/Math.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <QGraphicsPixmapItem>
#include <QScrollArea>
#include <QScrollBar>

ezQtImageScene::ezQtImageScene(QObject* pParent)
    : QGraphicsScene(pParent)
{
  m_pImageItem = nullptr;
  setItemIndexMethod(QGraphicsScene::NoIndex);
}

void ezQtImageScene::SetImage(QPixmap pixmap)
{
  if (m_pImageItem)
    delete m_pImageItem;

  m_pixmap = pixmap;
  m_pImageItem = addPixmap(m_pixmap);
  setSceneRect(0, 0, m_pixmap.width(), m_pixmap.height());
}



ezQtImageWidget::ezQtImageWidget(QWidget* parent, bool bShowButtons)
    : QWidget(parent)
{
  setupUi(this);
  m_pScene = new ezQtImageScene(GraphicsView);
  GraphicsView->setScene(m_pScene);

  m_fCurrentScale = 1.0f;

  if (!bShowButtons)
    ButtonBar->setVisible(false);
}

ezQtImageWidget::~ezQtImageWidget() {}

void ezQtImageWidget::SetImageSize(float fScale)
{
  if (m_fCurrentScale == fScale)
    return;

  m_fCurrentScale = fScale;
  ImageApplyScale();
}

void ezQtImageWidget::ScaleImage(float fFactor)
{
  float fPrevScale = m_fCurrentScale;
  m_fCurrentScale = ezMath::Clamp(m_fCurrentScale * fFactor, 0.2f, 5.0f);

  fFactor = m_fCurrentScale / fPrevScale;
  ImageApplyScale();
}

void ezQtImageWidget::ImageApplyScale()
{
  QTransform scale = QTransform::fromScale(m_fCurrentScale, m_fCurrentScale);
  GraphicsView->setTransform(scale);
}

void ezQtImageWidget::SetImage(QPixmap pixmap)
{
  m_pScene->SetImage(pixmap);
  ImageApplyScale();
}

void ezQtImageWidget::on_ButtonZoomIn_clicked()
{
  ScaleImage(1.25f);
}

void ezQtImageWidget::on_ButtonZoomOut_clicked()
{
  ScaleImage(0.75f);
}

void ezQtImageWidget::on_ButtonResetZoom_clicked()
{
  SetImageSize(1.0f);
}
