#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QBoxLayout>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtExposedBoneWidget::ezQtExposedBoneWidget()
{
  m_pRotWidget[0] = nullptr;
  m_pRotWidget[1] = nullptr;
  m_pRotWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (ezInt32 c = 0; c < 3; ++c)
  {
    m_pRotWidget[c] = new ezQtDoubleSpinBox(this);
    m_pRotWidget[c]->setMinimum(-ezMath::Infinity<double>());
    m_pRotWidget[c]->setMaximum(ezMath::Infinity<double>());
    m_pRotWidget[c]->setSingleStep(1.0);
    m_pRotWidget[c]->setAccelerated(true);
    m_pRotWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pRotWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pRotWidget[c]);

    connect(m_pRotWidget[c], SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
    connect(m_pRotWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void ezQtExposedBoneWidget::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  ezQtStandardPropertyWidget::SetSelection(items);
  EZ_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<ezExposedBone>(), "Selection does not match ezExposedBone.");
}

void ezQtExposedBoneWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(ezPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void ezQtExposedBoneWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(ezPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void ezQtExposedBoneWidget::SlotValueChanged()
{
  onBeginTemporary();

  auto obj = m_OldValue.Get<ezTypedObject>();
  ezExposedBone* pCopy = reinterpret_cast<ezExposedBone*>(ezReflectionSerializer::Clone(obj.m_pObject, obj.m_pType));

  {
    ezAngle x = ezAngle::MakeFromDegree(m_pRotWidget[0]->value());
    ezAngle y = ezAngle::MakeFromDegree(m_pRotWidget[1]->value());
    ezAngle z = ezAngle::MakeFromDegree(m_pRotWidget[2]->value());

    pCopy->m_Transform.m_qRotation = ezQuat::MakeFromEulerAngles(x, y, z);
  }

  ezVariant newValue;
  newValue.MoveTypedObject(pCopy, obj.m_pType);

  BroadcastValueChanged(newValue);
}

void ezQtExposedBoneWidget::OnInit()
{
}

void ezQtExposedBoneWidget::InternalSetValue(const ezVariant& value)
{
  if (value.GetReflectedType() != ezGetStaticRTTI<ezExposedBone>())
    return;

  const ezExposedBone* pBone = reinterpret_cast<const ezExposedBone*>(value.GetData());

  ezQtScopedBlockSignals b0(m_pRotWidget[0]);
  ezQtScopedBlockSignals b1(m_pRotWidget[1]);
  ezQtScopedBlockSignals b2(m_pRotWidget[2]);

  if (value.IsValid())
  {
    ezAngle x, y, z;
    pBone->m_Transform.m_qRotation.GetAsEulerAngles(x, y, z);

    m_pRotWidget[0]->setValue(x.GetDegree());
    m_pRotWidget[1]->setValue(y.GetDegree());
    m_pRotWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pRotWidget[0]->setValueInvalid();
    m_pRotWidget[1]->setValueInvalid();
    m_pRotWidget[2]->setValueInvalid();
  }
}
