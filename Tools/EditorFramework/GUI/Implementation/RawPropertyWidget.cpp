#include <PCH.h>
#include <EditorFramework/GUI/RawPropertyWidget.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <EditorFramework/GUI/CollapsibleGroupBox.moc.h>

void ezRawPropertyWidget::PropertyChangedHandler(const ezPropertyEditorBaseWidget::Event& e)
{
  m_PropertyChanged.Broadcast(e);
}

ezRawPropertyWidget::ezRawPropertyWidget(QWidget* pParent, const ezIReflectedTypeAccessor& accessor) : QWidget(pParent)
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  ezPropertyPath path;
  BuildUI(accessor, accessor.GetType(), path, m_pLayout);
}

void ezRawPropertyWidget::BuildUI(const ezIReflectedTypeAccessor& et, const ezRTTI* pType, ezPropertyPath& ParentPath, QLayout* pLayout)
{
  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    BuildUI(et, pParentType, ParentPath, pLayout);

  if (pType->GetProperties().GetCount() == 0)
    return;

  for (ezUInt32 i = 0; i < pType->GetProperties().GetCount(); ++i)
  {
    const ezAbstractProperty* pProp = pType->GetProperties()[i];

    ParentPath.PushBack(pProp->GetPropertyName());

    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType) && pProp->GetCategory() == ezPropertyCategory::Member)
    {
      const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProp);

      ezPropertyEditorBaseWidget* pNewWidget = nullptr;

      switch (pMember->GetSpecificType()->GetVariantType())
      {
      case ezVariant::Type::Bool:
        pNewWidget = new ezPropertyEditorCheckboxWidget(ParentPath, pProp->GetPropertyName(), this);
        break;

      case ezVariant::Type::Time:
      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, 1);
        break;

      case ezVariant::Type::Vector2:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, 2);
        break;
      case ezVariant::Type::Vector3:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, 3);
        break;
      case ezVariant::Type::Vector4:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, 4);
        break;

      case ezVariant::Type::Quaternion:
        pNewWidget = new ezPropertyEditorQuaternionWidget(ParentPath, pProp->GetPropertyName(), this);
        break;

      case ezVariant::Type::Int8:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, -127, 127);
        break;
      case ezVariant::Type::UInt8:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, 0, 255);
        break;
      case ezVariant::Type::Int16:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, -32767, 32767);
        break;
      case ezVariant::Type::UInt16:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, 0, 65535);
        break;
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, -2147483645, 2147483645);
        break;
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->GetPropertyName(), this, 0, 2147483645);
        break;

      case ezVariant::Type::String:
        pNewWidget = new ezPropertyEditorLineEditWidget(ParentPath, pProp->GetPropertyName(), this);
        break;

      case ezVariant::Type::Color:
        pNewWidget = new ezPropertyEditorColorWidget(ParentPath, pProp->GetPropertyName(), this);
        break;

      default:
        break;
      }

      if (pNewWidget)
      {
        ezStringBuilder sPropertyPath = ParentPath.GetPathString();

        m_PropertyWidgets[sPropertyPath] = pNewWidget;

        pLayout->addWidget(pNewWidget);
        pNewWidget->SetValue(et.GetValue(ParentPath));
        pNewWidget->setEnabled(!pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly));


        pNewWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezRawPropertyWidget::PropertyChangedHandler, this));
      }
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProp);

      ezPropertyEditorBaseWidget* pNewWidget = nullptr;
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum))
      {
        pNewWidget = new ezPropertyEditorEnumWidget(ParentPath, pProp->GetPropertyName(), this, pMember->GetSpecificType());
      }
      else
      {
        pNewWidget = new ezPropertyEditorBitflagsWidget(ParentPath, pProp->GetPropertyName(), this, pMember->GetSpecificType());
      }
      
      ezStringBuilder sPropertyPath = ParentPath.GetPathString();
      m_PropertyWidgets[sPropertyPath] = pNewWidget;

      pLayout->addWidget(pNewWidget);
      pNewWidget->SetValue(et.GetValue(ParentPath));
      pNewWidget->setEnabled(!pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly));

      pNewWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezRawPropertyWidget::PropertyChangedHandler, this));
    }
    else if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProp);

      ezCollapsibleGroupBox* pSubGroup = new ezCollapsibleGroupBox((QWidget*) pLayout->parent());
      pSubGroup->setTitle(QString::fromUtf8(pProp->GetPropertyName()));

      QVBoxLayout* pSubLayout = new QVBoxLayout(pSubGroup);
      pSubLayout->setSpacing(1);
      pSubGroup->setLayout(pSubLayout);

      pLayout->addWidget(pSubGroup);
      
      /// \todo read-only flag ?
      //pNewWidget->setEnabled(!pProp->GetFlags().IsSet(PropertyFlags::IsReadOnly));

      BuildUI(et, pMember->GetSpecificType(), ParentPath, pSubLayout);
    }

    ParentPath.PopBack();
  }
}

void ezRawPropertyWidget::ChangePropertyValue(const ezString& sPropertyPath, const ezVariant& value)
{
  auto it = m_PropertyWidgets.Find(sPropertyPath);

  if (it.IsValid())
  {
    it.Value()->SetValue(value);
  }
}

