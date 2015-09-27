#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
//#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
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
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>

/// *** CHECKBOX ***

ezPropertyEditorCheckboxWidget::ezPropertyEditorCheckboxWidget()
  : ezStandardPropertyBaseWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QCheckBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int))) != nullptr, "signal/slot connection failed");
}

void ezPropertyEditorCheckboxWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_pWidget->setTristate(false);
    m_pWidget->setChecked(value.ConvertTo<bool>() ? Qt::Checked : Qt::Unchecked);
  }
  else
  {
    m_pWidget->setTristate(true);
    m_pWidget->setCheckState(Qt::CheckState::PartiallyChecked);
  }
}

void ezPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* ev)
{
  QWidget::mousePressEvent(ev);

  m_pWidget->toggle();
}

void ezPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  if (state == Qt::PartiallyChecked)
  {
    QtScopedBlockSignals b(m_pWidget);

    m_pWidget->setCheckState(Qt::Checked);
    m_pWidget->setTristate(false);
  }

  BroadcastValueChanged((state != Qt::Unchecked) ? true : false);
}


/// *** DOUBLE SPINBOX ***

ezPropertyEditorDoubleSpinboxWidget::ezPropertyEditorDoubleSpinboxWidget(ezInt8 iNumComponents) : ezStandardPropertyBaseWidget()
{
  m_iNumComponents = iNumComponents;
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;
  m_pWidget[3] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (ezInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new QDoubleSpinBoxLessAnnoying(this);
    m_pWidget[c]->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setMaximum(ezMath::BasicType<double>::GetInfinity());
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
  QtScopedBlockSignals b0(m_pWidget[0]);
  QtScopedBlockSignals b1(m_pWidget[1]);
  QtScopedBlockSignals b2(m_pWidget[2]);
  QtScopedBlockSignals b3(m_pWidget[3]);

  if (value.IsValid())
  {
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
  else
  {
    switch (m_iNumComponents)
    {
    case 1:
      m_pWidget[0]->setValueInvalid();
      break;
    case 2:
      m_pWidget[0]->setValueInvalid();
      m_pWidget[1]->setValueInvalid();
      break;
    case 3:
      m_pWidget[0]->setValueInvalid();
      m_pWidget[1]->setValueInvalid();
      m_pWidget[2]->setValueInvalid();
      break;
    case 4:
      m_pWidget[0]->setValueInvalid();
      m_pWidget[1]->setValueInvalid();
      m_pWidget[2]->setValueInvalid();
      m_pWidget[3]->setValueInvalid();
      break;
    }
  }

}

void ezPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyBaseWidget::Event::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyBaseWidget::Event::Type::BeginTemporary);

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

ezPropertyEditorIntSpinboxWidget::ezPropertyEditorIntSpinboxWidget(ezInt32 iMinValue, ezInt32 iMaxValue) : ezStandardPropertyBaseWidget()
{
  m_bTemporaryCommand = false;
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QSpinBox(this);
  m_pWidget->setMinimum(iMinValue);
  m_pWidget->setMaximum(iMaxValue);
  m_pWidget->setSingleStep(1);
  m_pWidget->setAccelerated(true);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
  connect(m_pWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotValueChanged()));
}

void ezPropertyEditorIntSpinboxWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);
  m_pWidget->setValue(value.ConvertTo<ezInt32>());
}

void ezPropertyEditorIntSpinboxWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyBaseWidget::Event::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(m_pWidget->value());
}

void ezPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyBaseWidget::Event::Type::EndTemporary);

  m_bTemporaryCommand = false;
}


/// *** QUATERNION ***

ezPropertyEditorQuaternionWidget::ezPropertyEditorQuaternionWidget() : ezStandardPropertyBaseWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (ezInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new QDoubleSpinBoxLessAnnoying(this);
    m_pWidget[c]->setMinimum(-ezMath::BasicType<double>::GetInfinity());
    m_pWidget[c]->setMaximum(ezMath::BasicType<double>::GetInfinity());
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

void ezPropertyEditorQuaternionWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b0(m_pWidget[0]);
  QtScopedBlockSignals b1(m_pWidget[1]);
  QtScopedBlockSignals b2(m_pWidget[2]);

  const ezQuat qRot = value.ConvertTo<ezQuat>();
  ezAngle Yaw, Pitch, Roll;
  qRot.GetAsEulerAngles(Yaw, Pitch, Roll);

  m_pWidget[0]->setValue(Roll.GetDegree());
  m_pWidget[1]->setValue(Yaw.GetDegree());
  m_pWidget[2]->setValue(Pitch.GetDegree());
}

void ezPropertyEditorQuaternionWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyBaseWidget::Event::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezPropertyEditorQuaternionWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(ezPropertyBaseWidget::Event::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  ezAngle Roll = ezAngle::Degree(m_pWidget[0]->value());
  ezAngle Yaw = ezAngle::Degree(m_pWidget[1]->value());
  ezAngle Pitch = ezAngle::Degree(m_pWidget[2]->value());

  ezQuat qRot;
  qRot.SetFromEulerAngles(Yaw, Pitch, Roll);

  BroadcastValueChanged(qRot);
}

/// *** LINEEDIT ***

ezPropertyEditorLineEditWidget::ezPropertyEditorLineEditWidget() : ezStandardPropertyBaseWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
}

void ezPropertyEditorLineEditWidget::OnInit()
{
  if (m_pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr || m_pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    setEnabled(true);

    m_pWidget->setReadOnly(true);
    auto palette = m_pWidget->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    m_pWidget->setPalette(palette);
  }
}

void ezPropertyEditorLineEditWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(value.ConvertTo<ezString>().GetData()));
  }
}

void ezPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  BroadcastValueChanged(value.toUtf8().data());
}

void ezPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(m_pWidget->text().toUtf8().data());
}


/// *** ezPropertyEditorFileBrowserWidget ***

ezPropertyEditorFileBrowserWidget::ezPropertyEditorFileBrowserWidget() : ezPropertyEditorLineEditWidget()
{
  m_pButton = nullptr;
}

void ezPropertyEditorFileBrowserWidget::OnInit()
{
  ezPropertyEditorLineEditWidget::OnInit();

  const ezFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<ezFileBrowserAttribute>();
  EZ_ASSERT_DEV(pFileAttribute != nullptr, "ezPropertyEditorFileBrowserWidget was created without a ezFileBrowserAttribute!");

  m_pButton = new QToolButton(this);
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setText("...");

  m_pLayout->addWidget(m_pButton);

  connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked()));

  if (m_pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr || m_pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    if (m_pButton)
      m_pButton->setEnabled(false);
  }
}

void ezPropertyEditorFileBrowserWidget::on_BrowseFile_clicked()
{
  ezString sFile = m_pWidget->text().toUtf8().data();
  const ezFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<ezFileBrowserAttribute>();

  /*if (m_pAssetAttribute != nullptr)
  {
  ezAssetBrowserDlg dlg(this, sFile, m_pAssetAttribute->GetTypeFilter());
  if (dlg.exec() == 0)
  return;

  sFile = dlg.GetSelectedAssetGuid();

  if (sFile.IsEmpty())
  {
  sFile = dlg.GetSelectedAssetPathRelative();

  if (sFile.IsEmpty())
  {
  sFile = dlg.GetSelectedAssetPathAbsolute();

  ezEditorApp::GetInstance()->MakePathDataDirectoryRelative(sFile);
  }
  }

  if (sFile.IsEmpty())
  return;

  m_pWidget->setText(sFile.GetData());
  on_TextFinished_triggered();
  }
  else if (m_pFileAttribute)
  {

  }
  */
}


/// *** COLOR ***

ezColorButton::ezColorButton(QWidget* parent) : QFrame(parent)
{
  setAutoFillBackground(true);
}

void ezColorButton::SetColor(const ezVariant& color)
{
  if (color.IsValid())
  {
    ezColorGammaUB col = color.ConvertTo<ezColor>();

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(qol, Qt::SolidPattern));

    setPalette(pal);
  }
  else
  {
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(pal.foreground().color(), Qt::DiagCrossPattern));

    setPalette(pal);
  }
}

void ezColorButton::mouseReleaseEvent(QMouseEvent* event)
{
  emit clicked();
}

ezPropertyEditorColorWidget::ezPropertyEditorColorWidget() : ezStandardPropertyBaseWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new ezColorButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void ezPropertyEditorColorWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  m_OriginalValue = GetOldValue();
  m_pWidget->SetColor(value);
}

void ezPropertyEditorColorWidget::on_Button_triggered()
{
  Broadcast(ezPropertyBaseWidget::Event::Type::BeginTemporary);

  ezColor temp = ezColor::White;
  if (m_OriginalValue.IsValid())
    temp = m_OriginalValue.ConvertTo<ezColor>();

  ezUIServices::GetInstance()->ShowColorDialog(temp, true, this, SLOT(on_CurrentColor_changed(const QColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void ezPropertyEditorColorWidget::on_CurrentColor_changed(const QColor& color)
{
  const ezColor NewCol(ezColorGammaUB(color.red(), color.green(), color.blue(), color.alpha()));

  m_pWidget->SetColor(NewCol);

  BroadcastValueChanged(NewCol);
}

void ezPropertyEditorColorWidget::on_Color_reset()
{
  m_pWidget->SetColor(m_OriginalValue);
  Broadcast(ezPropertyBaseWidget::Event::Type::CancelTemporary);
}

void ezPropertyEditorColorWidget::on_Color_accepted()
{
  m_OriginalValue = GetOldValue();
  Broadcast(ezPropertyBaseWidget::Event::Type::EndTemporary);
}


/// *** ENUM COMBOBOX ***

ezPropertyEditorEnumWidget::ezPropertyEditorEnumWidget() : ezStandardPropertyBaseWidget()
{
  
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));
}

void ezPropertyEditorEnumWidget::OnInit()
{
  const ezRTTI* enumType = m_pProp->GetSpecificType();

  ezStringBuilder sTemp;
  const ezRTTI* pType = enumType;
  ezUInt32 uiCount = pType->GetProperties().GetCount();
  // Start at 1 to skip default value.
  for (ezUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != ezPropertyCategory::Constant)
      continue;

    const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(pProp);

    // If this is a qualified C++ name, skip everything before the last colon
    sTemp = pConstant->GetPropertyName();
    const char* szColon = sTemp.FindLastSubString(":");
    if (szColon != nullptr)
      szColon = szColon + 1;
    else
      szColon = sTemp;

    m_pWidget->addItem(QString::fromUtf8(szColon), pConstant->GetConstant().ConvertTo<ezInt64>());
  }
}

void ezPropertyEditorEnumWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    ezInt32 iIndex = m_pWidget->findData(value.ConvertTo<ezInt64>());
    EZ_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void ezPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  ezInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}


/// *** BITFLAGS COMBOBOX ***

ezPropertyEditorBitflagsWidget::ezPropertyEditorBitflagsWidget() : ezStandardPropertyBaseWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
  connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide()));
}

ezPropertyEditorBitflagsWidget::~ezPropertyEditorBitflagsWidget()
{
  m_Constants.Clear();
  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
}

void ezPropertyEditorBitflagsWidget::OnInit()
{
  const ezRTTI* enumType = m_pProp->GetSpecificType();

  const ezRTTI* pType = enumType;
  ezUInt32 uiCount = pType->GetProperties().GetCount();

  // Start at 1 to skip default value.
  for (ezUInt32 i = 1; i < uiCount; ++i)
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
}

void ezPropertyEditorBitflagsWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);
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

