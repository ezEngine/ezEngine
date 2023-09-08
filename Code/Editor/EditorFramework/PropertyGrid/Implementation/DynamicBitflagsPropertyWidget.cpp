#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicBitflagsPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicBitflags.h>

ezQtDynamicBitflagsPropertyWidget::ezQtDynamicBitflagsPropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow())), "show connection failed");
  EZ_VERIFY(connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide())), "hide connection failed");
}

ezQtDynamicBitflagsPropertyWidget::~ezQtDynamicBitflagsPropertyWidget()
{
  m_Constants.Clear();

  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
}

void ezQtDynamicBitflagsPropertyWidget::on_Menu_aboutToHide()
{
  ezInt64 iValue = 0;
  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = it.Value()->checkState() == Qt::Checked;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
      iValue |= it.Key();
    }
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);

  if (m_iCurrentBitflags != iValue)
  {
    m_iCurrentBitflags = iValue;
    BroadcastValueChanged(m_iCurrentBitflags);
  }
}

void ezQtDynamicBitflagsPropertyWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());

  // we build the menu dynamically because entries could change
  {
    ClearMenu();
    BuildMenu();
    FillInCheckedBoxes();
  }
}

void ezQtDynamicBitflagsPropertyWidget::OnInit()
{
  BuildMenu();
  FillInCheckedBoxes();
}

void ezQtDynamicBitflagsPropertyWidget::InternalSetValue(const ezVariant& value)
{
  ezQtScopedBlockSignals b(m_pWidget);
  m_iCurrentBitflags = value.ConvertTo<ezInt64>();

  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = (it.Key() & m_iCurrentBitflags) != 0;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
    }
    it.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);
}

void ezQtDynamicBitflagsPropertyWidget::SetAll(bool bChecked)
{
  for (auto& pCheckBox : m_Constants)
  {
    pCheckBox.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
}

void ezQtDynamicBitflagsPropertyWidget::ClearMenu()
{
  //for (auto action : m_pMenu->actions())
    //delete action;
  m_pMenu->clear();
  m_Constants.Clear();
}

void ezQtDynamicBitflagsPropertyWidget::BuildMenu()
{
  EZ_ASSERT_DEV(
    m_pProp->GetAttributeByType<ezDynamicBitflagsAttribute>() != nullptr, "ezQtDynamicBitflagsPropertyWidget was created without a ezDynamicBitflagsAttribute!");

  const ezDynamicBitflagsAttribute* pAttr = m_pProp->GetAttributeByType<ezDynamicBitflagsAttribute>();

  const auto& dbitflags = ezDynamicBitflags::GetDynamicBitflags(pAttr->GetDynamicBitflagsName());
  const auto& AllValues = dbitflags.GetAllValidValues();

  ezQtScopedBlockSignals bs(m_pWidget);

  // Start at 1 to skip default value.
  for (auto it = AllValues.GetIterator(); it.IsValid(); ++it)
  {
    ezString sContantName = it.Value();
    ezInt32 iConstant = it.Key();

    QWidgetAction* pAction = new QWidgetAction(m_pMenu);
    QCheckBox* pCheckBox = new QCheckBox(QString::fromUtf8(ezTranslate(sContantName)), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[(ezInt64)iConstant] = pCheckBox;
    m_pMenu->addAction(pAction);
  }

  // sets all bits to clear or set
  {
    QWidgetAction* pAllAction = new QWidgetAction(m_pMenu);
    m_pAllButton = new QPushButton(QString::fromUtf8("All"), m_pMenu);
    connect(m_pAllButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAll(true); });
    pAllAction->setDefaultWidget(m_pAllButton);
    m_pMenu->addAction(pAllAction);

    QWidgetAction* pClearAction = new QWidgetAction(m_pMenu);
    m_pClearButton = new QPushButton(QString::fromUtf8("Clear"), m_pMenu);
    connect(m_pClearButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAll(false); });
    pClearAction->setDefaultWidget(m_pClearButton);
    m_pMenu->addAction(pClearAction);
  }
}

void ezQtDynamicBitflagsPropertyWidget::FillInCheckedBoxes()
{
  if (m_iCurrentBitflags == 0)
    return;

  for (ezUInt32 i = 0; i < 64; ++i) // at max 64 bits
  {
    ezInt64 iValue = EZ_BIT(i);
    if (!m_Constants.Contains(iValue))
      continue;

    m_Constants[iValue]->setChecked((iValue & m_iCurrentBitflags) > 0);
  }
}
