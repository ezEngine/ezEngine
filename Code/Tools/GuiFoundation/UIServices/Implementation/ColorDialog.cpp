#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QWidget>
#include <QColor>
#include <QColorDialog>

void ezUIServices::ShowColorDialog(const ezColor& color, bool bAlpha, QWidget* pParent, const char* slotCurColChanged, const char* slotAccept, const char* slotReject)
{
  ezColorGammaUB gamma = color;

  QColor col;
  col.setRgb(gamma.r, gamma.g, gamma.b, gamma.a);

  m_pColorDlg = new QColorDialog (col, pParent);
  m_pColorDlg->move(m_ColorDlgPos);
  m_pColorDlg->setOption (QColorDialog::ShowAlphaChannel, bAlpha);
  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(currentColorChanged(const QColor&)), pParent, slotCurColChanged) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(accepted()), pParent, slotAccept) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(rejected()), pParent, slotReject) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(accepted()), this, SLOT(SlotColorDialogClosed())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(rejected()), this, SLOT(SlotColorDialogClosed())) != nullptr, "signal/slot connection failed");
  m_pColorDlg->open(pParent, slotCurColChanged);
}

void ezUIServices::SlotColorDialogClosed()
{
  m_ColorDlgPos = m_pColorDlg->pos();
  m_pColorDlg = nullptr;
}

