#include <PCH.h>
#include <EditorFramework/GUI/SimplePropertyGridWidget.moc.h>
#include <QPushButton>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <QScrollArea>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>

ezSimplePropertyGridWidget::ezSimplePropertyGridWidget(QWidget* pParent) : QWidget(pParent)
{
  m_pScrollArea = new QScrollArea(this);
  m_pScrollArea->setContentsMargins(0, 0, 0, 0);

  QVBoxLayout* pLayout2 = new QVBoxLayout(this);
  pLayout2->setSpacing(1);
  pLayout2->setMargin(1);
  this->setLayout(pLayout2);
  pLayout2->addWidget(m_pScrollArea);

  m_pMainContent = new QWidget(this);
  m_pScrollArea->setWidget(m_pMainContent);
  m_pScrollArea->setWidgetResizable(true);
  m_pMainContent->setBackgroundRole(QPalette::ColorRole::Background);

  m_pLayout = new QVBoxLayout(m_pMainContent);
  m_pLayout->setSpacing(1);
  m_pLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  m_pMainContent->setLayout(m_pLayout);
}

ezSimplePropertyGridWidget::~ezSimplePropertyGridWidget()
{
}

void ezSimplePropertyGridWidget::ClearProperties()
{
  m_Properties.Clear();

  m_pMainContent->deleteLater();

  m_pMainContent = new QWidget(this);
  m_pScrollArea->setWidget(m_pMainContent);
  m_pScrollArea->setWidgetResizable(true);
  m_pMainContent->setBackgroundRole(QPalette::ColorRole::Background);

  m_pLayout = new QVBoxLayout(m_pMainContent);
  m_pLayout->setSpacing(1);
  m_pLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  m_pMainContent->setLayout(m_pLayout);
}

void ezSimplePropertyGridWidget::SlotPropertyChanged()
{
  for (ezUInt32 p = 0; p < m_Properties.GetCount(); ++p)
  {
    ezVariant::Type::Enum type = m_Properties[p].m_Value.GetType();

    if (m_Properties[p].m_Value.IsA<bool>())
    {
      m_Properties[p].m_Value = ((QCheckBox*) m_Properties[p].m_pWidget)->isChecked();
    }
    else
    if (m_Properties[p].m_Value.IsA<float>() || m_Properties[p].m_Value.IsA<double>())
    {
      m_Properties[p].m_Value = (float) ((QDoubleSpinBox*) m_Properties[p].m_pWidget)->value();
    }
    else
    if (m_Properties[p].m_Value.IsA<ezInt8>() || m_Properties[p].m_Value.IsA<ezUInt8>() ||
        m_Properties[p].m_Value.IsA<ezInt16>() || m_Properties[p].m_Value.IsA<ezUInt16>() ||
        m_Properties[p].m_Value.IsA<ezInt32>() || m_Properties[p].m_Value.IsA<ezUInt32>() ||
        m_Properties[p].m_Value.IsA<ezInt64>() || m_Properties[p].m_Value.IsA<ezUInt64>())
    {
      m_Properties[p].m_Value = (ezInt32) ((QSpinBox*) m_Properties[p].m_pWidget)->value();
    }
    else
    if (m_Properties[p].m_Value.IsA<ezString>())
    {
      m_Properties[p].m_Value = ((QLineEdit*) m_Properties[p].m_pWidget)->text().toUtf8().data();
    }

    if (m_Properties[p].m_pValue != nullptr)
      *m_Properties[p].m_pValue = m_Properties[p].m_Value.ConvertTo(m_Properties[p].m_pValue->GetType());

    // make sure the original type is preserved
    m_Properties[p].m_Value = m_Properties[p].m_Value.ConvertTo(type);
  }

  emit value_changed();
}

QWidget* ezSimplePropertyGridWidget::CreateControl(Property& Prop, QWidget* pWidget)
{
  Prop.m_pWidget = pWidget;

  QWidget* pControl = new QWidget(this);

  QHBoxLayout* m_pLayout = new QHBoxLayout(pControl);
  m_pLayout->setMargin(0);
  pControl->setLayout(m_pLayout);

  QLabel* m_pLabel = new QLabel(pControl);
  m_pLabel->setText(QString::fromUtf8(Prop.m_sName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(pWidget);

  m_pLabel->setEnabled(!Prop.m_bReadOnly);
  pWidget->setEnabled(!Prop.m_bReadOnly);

  return pControl;
}

QWidget* ezSimplePropertyGridWidget::CreateCheckbox(Property& Prop)
{
  QCheckBox* pWidget = new QCheckBox();
  pWidget->setChecked(Prop.m_Value.ConvertTo<bool>());

  EZ_VERIFY(connect(pWidget, SIGNAL(stateChanged(int)), this, SLOT(SlotPropertyChanged())) != nullptr, "signal/slot connection failed");

  return CreateControl(Prop, pWidget);
}

QWidget* ezSimplePropertyGridWidget::CreateDoubleSpinbox(Property& Prop)
{
  QDoubleSpinBox* pWidget = new QDoubleSpinBox();
  pWidget->setMinimum(-ezMath::BasicType<double>::GetInfinity());
  pWidget->setMaximum( ezMath::BasicType<double>::GetInfinity());
  pWidget->setSingleStep(1.0);
  pWidget->setAccelerated(true);
  pWidget->setDecimals(8);
  pWidget->setValue(Prop.m_Value.ConvertTo<double>());

  EZ_VERIFY(connect(pWidget, SIGNAL(editingFinished()), this, SLOT(SlotPropertyChanged())) != nullptr, "signal/slot connection failed");

  return CreateControl(Prop, pWidget);
}

QWidget* ezSimplePropertyGridWidget::CreateSpinbox(Property& Prop)
{
  QSpinBox* pWidget = new QSpinBox();
  pWidget->setMinimum(-2147483645);
  pWidget->setMaximum( 2147483645);
  pWidget->setSingleStep(1);
  pWidget->setAccelerated(true);
  pWidget->setValue(Prop.m_Value.ConvertTo<ezInt32>());

  EZ_VERIFY(connect(pWidget, SIGNAL(editingFinished()), this, SLOT(SlotPropertyChanged())) != nullptr, "signal/slot connection failed");

  return CreateControl(Prop, pWidget);
}

QWidget* ezSimplePropertyGridWidget::CreateLineEdit(Property& Prop)
{
  QLineEdit* pWidget = new QLineEdit();
  pWidget->setText(QString::fromUtf8(Prop.m_Value.ConvertTo<ezString>()));

  EZ_VERIFY(connect(pWidget, SIGNAL(editingFinished()), this, SLOT(SlotPropertyChanged())) != nullptr, "signal/slot connection failed");

  return CreateControl(Prop, pWidget);
}

void ezSimplePropertyGridWidget::BeginProperties()
{
  ClearProperties();
}

void ezSimplePropertyGridWidget::AddProperty(const char* szName, const ezVariant& value, ezVariant* pValue, bool bReadOnly)
{
  Property p;
  p.m_sName = szName;
  p.m_Value = value;
  p.m_pValue = pValue;
  p.m_bReadOnly = bReadOnly;

  m_Properties.PushBack(p);
}

void ezSimplePropertyGridWidget::EndProperties()
{
  if (m_Properties.IsEmpty())
    return;

  for (ezUInt32 p = 0; p < m_Properties.GetCount(); ++p)
  {
    Property& Prop = m_Properties[p];

    QWidget* pNewWidget = nullptr;

    if (Prop.m_Value.IsA<bool>())
    {
      pNewWidget = CreateCheckbox(Prop);
    }

    if (Prop.m_Value.IsA<float>() || Prop.m_Value.IsA<double>())
    {
      pNewWidget = CreateDoubleSpinbox(Prop);
    }

    if (Prop.m_Value.IsA<ezInt8>()  || Prop.m_Value.IsA<ezUInt8>()  ||
        Prop.m_Value.IsA<ezInt16>() || Prop.m_Value.IsA<ezUInt16>() ||
        Prop.m_Value.IsA<ezInt32>() || Prop.m_Value.IsA<ezUInt32>() ||
        Prop.m_Value.IsA<ezInt64>() || Prop.m_Value.IsA<ezUInt64>())
    {
      pNewWidget = CreateSpinbox(Prop);
    }

    if (Prop.m_Value.IsA<ezString>())
    {
      pNewWidget = CreateLineEdit(Prop);
    }

    if (Prop.m_pWidget)
    {
      m_pLayout->addWidget(pNewWidget);
    }
  }

  QSpacerItem* pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  m_pLayout->addSpacerItem(pSpacer);
}
