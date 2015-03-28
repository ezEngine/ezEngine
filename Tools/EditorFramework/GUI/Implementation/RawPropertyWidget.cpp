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
  BuildUI(accessor, accessor.GetReflectedTypeHandle().GetType(), path, m_pLayout);
}

void ezRawPropertyWidget::BuildUI(const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath, QLayout* pLayout)
{
  ezReflectedTypeHandle hParent = pType->GetParentTypeHandle();
  if (!hParent.IsInvalidated())
    BuildUI(et, hParent.GetType(), ParentPath, pLayout);

  if (pType->GetPropertyCount() == 0)
    return;

  for (ezUInt32 i = 0; i < pType->GetPropertyCount(); ++i)
  {
    const ezReflectedProperty* pProp = pType->GetPropertyByIndex(i);

    if (pProp->m_Flags.IsAnySet(PropertyFlags::IsPOD))
    {
      ParentPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

      ezPropertyEditorBaseWidget* pNewWidget = nullptr;

      switch (pProp->m_Type)
      {
      case ezVariant::Type::Bool:
        pNewWidget = new ezPropertyEditorCheckboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      case ezVariant::Type::Time:
      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 1);
        break;

      case ezVariant::Type::Vector2:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 2);
        break;
      case ezVariant::Type::Vector3:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 3);
        break;
      case ezVariant::Type::Vector4:
        pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 4);
        break;

      case ezVariant::Type::Int8:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, -127, 127);
        break;
      case ezVariant::Type::UInt8:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 0, 255);
        break;
      case ezVariant::Type::Int16:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, -32767, 32767);
        break;
      case ezVariant::Type::UInt16:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 0, 65535);
        break;
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, -2147483645, 2147483645);
        break;
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
        pNewWidget = new ezPropertyEditorIntSpinboxWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, 0, 2147483645);
        break;

      case ezVariant::Type::String:
        pNewWidget = new ezPropertyEditorLineEditWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      case ezVariant::Type::Color:
        pNewWidget = new ezPropertyEditorColorWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this);
        break;

      default:
        break;
      }

      if (pNewWidget)
      {
        ezString sPropertyPath = ezToolsReflectionUtils::GetStringFromPropertyPath(ParentPath);
        m_PropertyWidgets[sPropertyPath] = pNewWidget;

        pLayout->addWidget(pNewWidget);
        pNewWidget->SetValue(et.GetValue(ParentPath));
        pNewWidget->setEnabled(!pProp->m_Flags.IsSet(PropertyFlags::IsReadOnly));


        pNewWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezRawPropertyWidget::PropertyChangedHandler, this));
      }

      ParentPath.PopBack();
    }
    else if (pProp->m_Flags.IsAnySet(PropertyFlags::IsEnum | PropertyFlags::IsBitflags))
    {
      ParentPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

      ezPropertyEditorBaseWidget* pNewWidget = nullptr;
      if (pProp->m_Flags.IsAnySet(PropertyFlags::IsEnum))
      {
        pNewWidget = new ezPropertyEditorEnumWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, pProp->m_hTypeHandle);
      }
      else
      {
        pNewWidget = new ezPropertyEditorBitflagsWidget(ParentPath, pProp->m_sPropertyName.GetString().GetData(), this, pProp->m_hTypeHandle);
      }
      
      ezString sPropertyPath = ezToolsReflectionUtils::GetStringFromPropertyPath(ParentPath);
      m_PropertyWidgets[sPropertyPath] = pNewWidget;

      pLayout->addWidget(pNewWidget);
      pNewWidget->SetValue(et.GetValue(ParentPath));

      pNewWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezRawPropertyWidget::PropertyChangedHandler, this));
      
      ParentPath.PopBack();
    }
    else
    {
      ParentPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

      ezCollapsibleGroupBox* pSubGroup = new ezCollapsibleGroupBox((QWidget*) pLayout->parent());
      pSubGroup->setTitle(QString::fromUtf8(pProp->m_sPropertyName.GetString().GetData()));

      QVBoxLayout* pSubLayout = new QVBoxLayout(pSubGroup);
      pSubLayout->setSpacing(1);
      pSubGroup->setLayout(pSubLayout);

      pLayout->addWidget(pSubGroup);

      BuildUI(et, pProp->m_hTypeHandle.GetType(), ParentPath, pSubLayout);

      ParentPath.PopBack();
    }
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

