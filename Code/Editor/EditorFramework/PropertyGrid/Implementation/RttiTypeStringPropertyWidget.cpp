#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/RttiTypeStringPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

ezQtRttiTypeStringPropertyWidget::ezQtRttiTypeStringPropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Select Type");
  m_pButton->setObjectName("Button");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addWidget(m_pButton);
}

void ezQtRttiTypeStringPropertyWidget::OnInit()
{
  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(true);
  connect(m_pMenu, &QMenu::aboutToShow, this, &ezQtRttiTypeStringPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
  m_pButton->setObjectName("Button");

  connect(&m_TypeMenu, &ezQtTypeMenu::TypeSelected, this, &ezQtRttiTypeStringPropertyWidget::OnTypeSelected);
}

void ezQtRttiTypeStringPropertyWidget::InternalSetValue(const ezVariant& value)
{
  const ezString sTypeName = value.ConvertTo<ezString>();

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(sTypeName);

  const ezCategoryAttribute* pCatA = pRtti->GetAttributeByType<ezCategoryAttribute>();
  const ezColorAttribute* pColA = pRtti->GetAttributeByType<ezColorAttribute>();

  ezColor iconColor = ezColor::MakeZero();

  if (pColA)
  {
    iconColor = pColA->GetColor();
  }
  else if (pCatA && iconColor == ezColor::MakeZero())
  {
    iconColor = ezColorScheme::GetCategoryColor(pCatA->GetCategory(), ezColorScheme::CategoryColorUsage::MenuEntryIcon);
  }

  ezStringBuilder sIconName;
  sIconName.Set(":/TypeIcons/", sTypeName, ".svg");
  const QIcon actionIcon = ezQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

  m_pButton->setText(sTypeName.GetData());
  m_pButton->setIcon(actionIcon);
}

void ezQtRttiTypeStringPropertyWidget::onMenuAboutToShow()
{
  if (m_pMenu->isEmpty())
  {
    const ezRttiTypeStringAttribute* pTypeAttr = m_pProp->GetAttributeByType<ezRttiTypeStringAttribute>();
    const ezRTTI* pBaseType = ezRTTI::FindTypeByName(pTypeAttr->GetBaseType());

    m_TypeMenu.FillMenu(m_pMenu, pBaseType, true, false);
  }
}

void ezQtRttiTypeStringPropertyWidget::OnTypeSelected(QString sTypeName)
{
  const ezString typeName = sTypeName.toUtf8().data();

  BroadcastValueChanged(typeName);
}
