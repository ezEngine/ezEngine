#include <PCH.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <QLineEdit>
#include <QLabel>
#include <QKeyEvent>
#include <QSpinBox>
#include <QPushButton>
#include <QColor>
#include <QComboBox>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QWidgetAction>
#include <QToolButton>
#include <QFileDialog>
#include <EditorFramework/GUI/QtHelpers.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>

/// *** BASE ***

ezPropertyEditorBaseWidget::ezPropertyEditorBaseWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : QWidget(pParent)
{
  m_szDisplayName = szName;
  m_PropertyPath = path;
}

ezPropertyEditorBaseWidget::~ezPropertyEditorBaseWidget()
{
}

void ezPropertyEditorBaseWidget::SetValue(const ezVariant& value)
{
  m_OldValue = value;

  InternalSetValue(value);
}

void ezPropertyEditorBaseWidget::Broadcast(Event::Type type)
{
  Event ed;
  ed.m_Type = type;
  ed.m_pPropertyPath =  &m_PropertyPath;

  m_Events.Broadcast(ed);
}

void ezPropertyEditorBaseWidget::BroadcastValueChanged(const ezVariant& NewValue)
{
  if (NewValue == m_OldValue)
    return;

  m_OldValue = NewValue;

  Event ed;
  ed.m_Type = Event::Type::ValueChanged;
  ed.m_pPropertyPath = &m_PropertyPath;
  ed.m_Value = NewValue;

  m_Events.Broadcast(ed);
}


/// *** CHECKBOX ***

ezPropertyEditorCheckboxWidget::ezPropertyEditorCheckboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QCheckBox(this);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int))) != nullptr, "signal/slot connection failed");
}

void ezPropertyEditorCheckboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b(m_pWidget);
  m_pWidget->setChecked(value.ConvertTo<bool>() ? Qt::Checked : Qt::Unchecked);
}

void ezPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* ev)
{
  QWidget::mousePressEvent(ev);

  m_pWidget->toggle();
}

void ezPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  BroadcastValueChanged((state == Qt::Checked) ? true : false);
}


/// *** DOUBLE SPINBOX ***

ezPropertyEditorDoubleSpinboxWidget::ezPropertyEditorDoubleSpinboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent, ezInt8 iNumComponents) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_iNumComponents= iNumComponents;
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;
  m_pWidget[3] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(m_iNumComponents);
  m_pLabel->setSizePolicy(policy);
  m_pLayout->addWidget(m_pLabel);

  for (ezInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new QDoubleSpinBox(this);
    m_pWidget[c]->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setMaximum( ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDecimals(3);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezPropertyEditorDoubleSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b0 (m_pWidget[0]);
  ezQtBlockSignals b1 (m_pWidget[1]);
  ezQtBlockSignals b2 (m_pWidget[2]);
  ezQtBlockSignals b3 (m_pWidget[3]);

  switch (m_iNumComponents)
  {
  case 1:
    m_pWidget[0]->setValue(value.ConvertTo<double>());
    break;
  case 2:
    m_pWidget[0]->setValue(value.ConvertTo<ezVec2>().x);
    m_pWidget[1]->setValue(value.ConvertTo<ezVec2>().y);
    break;
  case 3:
    m_pWidget[0]->setValue(value.ConvertTo<ezVec3>().x);
    m_pWidget[1]->setValue(value.ConvertTo<ezVec3>().y);
    m_pWidget[2]->setValue(value.ConvertTo<ezVec3>().z);
    break;
  case 4:
    m_pWidget[0]->setValue(value.ConvertTo<ezVec4>().x);
    m_pWidget[1]->setValue(value.ConvertTo<ezVec4>().y);
    m_pWidget[2]->setValue(value.ConvertTo<ezVec4>().z);
    m_pWidget[3]->setValue(value.ConvertTo<ezVec4>().w);
    break;
  }
}

void ezPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEditorBaseWidget::Event::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEditorBaseWidget::Event::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  switch (m_iNumComponents)
  {
  case 1:
    BroadcastValueChanged(m_pWidget[0]->value());
    break;
  case 2:
    BroadcastValueChanged(ezVec2(m_pWidget[0]->value(), m_pWidget[1]->value()));
    break;
  case 3:
    BroadcastValueChanged(ezVec3(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value()));
    break;
  case 4:
    BroadcastValueChanged(ezVec4(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value()));
    break;
  }
}

/// *** INT SPINBOX ***

ezPropertyEditorIntSpinboxWidget::ezPropertyEditorIntSpinboxWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent, ezInt32 iMinValue, ezInt32 iMaxValue) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_bTemporaryCommand = false;
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QSpinBox(this);
  m_pWidget->setMinimum(iMinValue);
  m_pWidget->setMaximum(iMaxValue);
  m_pWidget->setSingleStep(1);
  m_pWidget->setAccelerated(true);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  connect(m_pWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotValueChanged()));
}

void ezPropertyEditorIntSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_pWidget->setValue(value.ConvertTo<ezInt32>());
}

void ezPropertyEditorIntSpinboxWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyEditorBaseWidget::Event::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(m_pWidget->value());
}

void ezPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEditorBaseWidget::Event::Type::EndTemporary);

  m_bTemporaryCommand = false;
}


/// *** LINEEDIT ***

ezPropertyEditorLineEditWidget::ezPropertyEditorLineEditWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pButton = new QToolButton(this);
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setText("...");

  m_pWidget = new QLineEdit(this);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
  connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked()));
}

void ezPropertyEditorLineEditWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_pWidget->setText(QString::fromUtf8(value.ConvertTo<ezString>().GetData()));
}

void ezPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  BroadcastValueChanged(value.toUtf8().data());
}

void ezPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(m_pWidget->text().toUtf8().data());
}

void ezPropertyEditorLineEditWidget::on_BrowseFile_clicked()
{
  ezString sFile = m_pWidget->text().toUtf8().data();

  ezAssetBrowserDlg dlg(this, sFile, "");
  if (dlg.exec() == 0)
    return;

  sFile = dlg.GetSelectedPath();

  if (sFile.IsEmpty())
    return;

  ezEditorApp::GetInstance()->MakePathDataDirectoryRelative(sFile);

  m_pWidget->setText(sFile.GetData());
  on_TextFinished_triggered();
}

/// *** COLOR ***

ezPropertyEditorColorWidget::ezPropertyEditorColorWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QPushButton(this);

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void ezPropertyEditorColorWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  m_CurrentColor = value.ConvertTo<ezColor>();

  QColor col;
  col.setRgbF(m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, m_CurrentColor.a);
  
  const QString COLOR_STYLE("QPushButton { background-color : %1 }");
  m_pWidget->setStyleSheet(COLOR_STYLE.arg(col.name()));
}

void ezPropertyEditorColorWidget::on_Button_triggered()
{
  QColor col;
  col.setRgbF(m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, m_CurrentColor.a);

  Broadcast(ezPropertyEditorBaseWidget::Event::Type::BeginTemporary);

  ezUIServices::GetInstance()->ShowColorDialog(m_CurrentColor, true, this, SLOT(on_CurrentColor_changed(const QColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void ezPropertyEditorColorWidget::on_CurrentColor_changed(const QColor& color)
{
  const ezColor NewCol(color.redF(), color.greenF(), color.blueF(), color.alphaF());

  const QString COLOR_STYLE("QPushButton { background-color : %1 }");
  m_pWidget->setStyleSheet(COLOR_STYLE.arg(color.name()));

  m_CurrentColor = NewCol;

  BroadcastValueChanged(NewCol);
}

void ezPropertyEditorColorWidget::on_Color_reset()
{
  QColor color;
  color.setRgbF(m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, m_CurrentColor.a);

  const QString COLOR_STYLE("QPushButton { background-color : %1 }");
  m_pWidget->setStyleSheet(COLOR_STYLE.arg(color.name()));

  Broadcast(ezPropertyEditorBaseWidget::Event::Type::CancelTemporary);
}

void ezPropertyEditorColorWidget::on_Color_accepted()
{

  Broadcast(ezPropertyEditorBaseWidget::Event::Type::EndTemporary);
}


/// *** ENUM COMBOBOX ***

ezPropertyEditorEnumWidget::ezPropertyEditorEnumWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent, const ezRTTI* enumType) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QComboBox(this);

  ezStringBuilder sTemp;

  const ezRTTI* pType = enumType;
  ezUInt32 uiCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != ezPropertyCategory::Constant)
      continue;

    const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(pProp);

    // If this is a qualified C++ name, skip everything before the last colon
    sTemp = pConstant->GetPropertyName();
    const char* szColon = sTemp.FindLastSubString(":");
    if (szColon != nullptr)
      sTemp = szColon + 1;

    m_pWidget->addItem(QString::fromUtf8(sTemp.GetData()), pConstant->GetConstant().ConvertTo<ezInt64>());
  }

  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL( currentIndexChanged(int) ), this, SLOT( on_CurrentEnum_changed(int) ) );
}

void ezPropertyEditorEnumWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
  ezInt32 iIndex = m_pWidget->findData(value.ConvertTo<ezInt64>());
  EZ_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
  m_pWidget->setCurrentIndex(iIndex);
}

void ezPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  ezInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}


/// *** BITFLAGS COMBOBOX ***

ezPropertyEditorBitflagsWidget::ezPropertyEditorBitflagsWidget(const ezPropertyPath& path, const char* szName, QWidget* pParent, const ezRTTI* enumType) : ezPropertyEditorBaseWidget(path, szName, pParent)
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pLabel = new QLabel(this);
  m_pLabel->setText(QString::fromUtf8(szName));
  m_pLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

  m_pWidget = new QPushButton(this);
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  const ezRTTI* pType = enumType;
  ezUInt32 uiCount = pType->GetProperties().GetCount();
  
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != ezPropertyCategory::Constant)
      continue;

    const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(pProp);

    QWidgetAction* pAction = new QWidgetAction(m_pMenu);
    QCheckBox* pCheckBox = new QCheckBox(QString::fromUtf8(pConstant->GetPropertyName()), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<ezInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
  }
  
  QSizePolicy policy = m_pLabel->sizePolicy();
  policy.setHorizontalStretch(1);
  m_pLabel->setSizePolicy(policy);
  policy.setHorizontalStretch(2);
  m_pWidget->setSizePolicy(policy);

  m_pLayout->addWidget(m_pLabel);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL( aboutToShow() ), this, SLOT( on_Menu_aboutToShow() ));
  connect(m_pMenu, SIGNAL( aboutToHide() ), this, SLOT( on_Menu_aboutToHide() ));
}

ezPropertyEditorBitflagsWidget::~ezPropertyEditorBitflagsWidget()
{
  m_Constants.Clear();
  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
}

void ezPropertyEditorBitflagsWidget::InternalSetValue(const ezVariant& value)
{
  ezQtBlockSignals b (m_pWidget);
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

void ezPropertyEditorBitflagsWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void ezPropertyEditorBitflagsWidget::on_Menu_aboutToHide()
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

