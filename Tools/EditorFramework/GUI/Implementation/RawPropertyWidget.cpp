#include <PCH.h>
#include <EditorFramework/GUI/RawPropertyWidget.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <EditorFramework/GUI/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include "../AddSubElementButton.moc.h"

const ezRTTI* GetCommonBaseType(const ezRTTI* pRtti1, const ezRTTI* pRtti2)
{
  if (pRtti2 == nullptr)
    return nullptr;

  while (pRtti1 != nullptr)
  {
    const ezRTTI* pRtti2Parent = pRtti2;

    while (pRtti2Parent != nullptr)
    {
      if (pRtti1 == pRtti2Parent)
        return pRtti2Parent;

      pRtti2Parent = pRtti2Parent->GetParentType();
    }

    pRtti1 = pRtti1->GetParentType();
  }

  return nullptr;
}

const ezRTTI* GetCommonBaseType(const ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8>& items, bool bEditorProperty)
{
  const ezRTTI* pSubtype = nullptr;

  for (const auto& item : items)
  {
    const auto& accessor = bEditorProperty ? item.m_pObject->GetEditorTypeAccessor() : item.m_pObject->GetTypeAccessor();

    if (pSubtype == nullptr)
      pSubtype = accessor.GetType();
    else
    {
      pSubtype = GetCommonBaseType(pSubtype, accessor.GetType());
    }
  }

  return pSubtype;
}

void ezRawPropertyWidget::PropertyChangedHandler(const ezPropertyEditorBaseWidget::Event& e)
{
  m_PropertyChanged.Broadcast(e);
}

ezRawPropertyWidget::ezRawPropertyWidget(QWidget* pParent, const ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8>& items, bool bEditorProperty) : QWidget(pParent)
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(5, 0, 0, 0);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  const ezRTTI* pSubtype = GetCommonBaseType(items, bEditorProperty);

  ezPropertyPath path;
  BuildUI(items, bEditorProperty, pSubtype, path, m_pLayout);
}

void ezRawPropertyWidget::BuildUI(const ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8>& items, bool bEditorProperty, const ezRTTI* pType, ezPropertyPath& ParentPath, QLayout* pLayout)
{
  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    BuildUI(items, bEditorProperty, pParentType, ParentPath, pLayout);

  if (pType->GetProperties().GetCount() == 0)
    return;

  for (ezUInt32 i = 0; i < pType->GetProperties().GetCount(); ++i)
  {
    const ezAbstractProperty* pProp = pType->GetProperties()[i];

    if (pProp->GetFlags().IsSet(ezPropertyFlags::Hidden))
      continue;

    ParentPath.PushBack(pProp->GetPropertyName());

    switch (pProp->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
        {
          const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProp);

          ezPropertyEditorBaseWidget* pNewWidget = nullptr;

          switch (pMember->GetSpecificType()->GetVariantType())
          {
          case ezVariant::Type::Bool:
            pNewWidget = new ezPropertyEditorCheckboxWidget(pProp->GetPropertyName(), this);
            break;

          case ezVariant::Type::Time:
          case ezVariant::Type::Float:
          case ezVariant::Type::Double:
            pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(pProp->GetPropertyName(), this, 1);
            break;

          case ezVariant::Type::Vector2:
            pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(pProp->GetPropertyName(), this, 2);
            break;
          case ezVariant::Type::Vector3:
            pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(pProp->GetPropertyName(), this, 3);
            break;
          case ezVariant::Type::Vector4:
            pNewWidget = new ezPropertyEditorDoubleSpinboxWidget(pProp->GetPropertyName(), this, 4);
            break;

          case ezVariant::Type::Quaternion:
            pNewWidget = new ezPropertyEditorQuaternionWidget(pProp->GetPropertyName(), this);
            break;

          case ezVariant::Type::Int8:
            pNewWidget = new ezPropertyEditorIntSpinboxWidget(pProp->GetPropertyName(), this, -127, 127);
            break;
          case ezVariant::Type::UInt8:
            pNewWidget = new ezPropertyEditorIntSpinboxWidget(pProp->GetPropertyName(), this, 0, 255);
            break;
          case ezVariant::Type::Int16:
            pNewWidget = new ezPropertyEditorIntSpinboxWidget(pProp->GetPropertyName(), this, -32767, 32767);
            break;
          case ezVariant::Type::UInt16:
            pNewWidget = new ezPropertyEditorIntSpinboxWidget(pProp->GetPropertyName(), this, 0, 65535);
            break;
          case ezVariant::Type::Int32:
          case ezVariant::Type::Int64:
            pNewWidget = new ezPropertyEditorIntSpinboxWidget(pProp->GetPropertyName(), this, -2147483645, 2147483645);
            break;
          case ezVariant::Type::UInt32:
          case ezVariant::Type::UInt64:
            pNewWidget = new ezPropertyEditorIntSpinboxWidget(pProp->GetPropertyName(), this, 0, 2147483645);
            break;

          case ezVariant::Type::String:
            pNewWidget = new ezPropertyEditorLineEditWidget(pProp->GetPropertyName(), this);
            break;

          case ezVariant::Type::Color:
            pNewWidget = new ezPropertyEditorColorWidget(pProp->GetPropertyName(), this);
            break;

          default:
            break;
          }

          if (pNewWidget)
          {
            ezStringBuilder sPropertyPath = ParentPath.GetPathString();

            m_PropertyWidgets[sPropertyPath] = pNewWidget;

            pLayout->addWidget(pNewWidget);
            pNewWidget->Init(items, ParentPath, bEditorProperty);
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
            pNewWidget = new ezPropertyEditorEnumWidget(pProp->GetPropertyName(), this, pMember->GetSpecificType());
          }
          else
          {
            pNewWidget = new ezPropertyEditorBitflagsWidget(pProp->GetPropertyName(), this, pMember->GetSpecificType());
          }

          ezStringBuilder sPropertyPath = ParentPath.GetPathString();
          m_PropertyWidgets[sPropertyPath] = pNewWidget;

          pLayout->addWidget(pNewWidget);
          pNewWidget->Init(items, ParentPath, bEditorProperty);
          pNewWidget->setEnabled(!pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly));

          pNewWidget->m_Events.AddEventHandler(ezMakeDelegate(&ezRawPropertyWidget::PropertyChangedHandler, this));
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
        else
        {
          const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProp);

          ezCollapsibleGroupBox* pSubGroup = new ezCollapsibleGroupBox((QWidget*)pLayout->parent());
          pSubGroup->setTitle(QString::fromUtf8(pProp->GetPropertyName()));

          QVBoxLayout* pSubLayout = new QVBoxLayout(pSubGroup);
          pSubLayout->setSpacing(1);
          pSubLayout->setContentsMargins(5, 0, 0, 0);
          pSubGroup->setInnerLayout(pSubLayout);

          pLayout->addWidget(pSubGroup);

          /// \todo read-only flag ?
          //pNewWidget->setEnabled(!pProp->GetFlags().IsSet(PropertyFlags::IsReadOnly));

          BuildUI(items, bEditorProperty, pMember->GetSpecificType(), ParentPath, pSubLayout);
        }

      }
      break;

    case ezPropertyCategory::Set:
    case ezPropertyCategory::Array:
      {
        QVBoxLayout* pElementsLayout;

        {
          ezCollapsibleGroupBox* pSubGroup = new ezCollapsibleGroupBox((QWidget*)pLayout->parent());
          pSubGroup->setTitle(QString::fromUtf8(pProp->GetPropertyName()));

          pElementsLayout = new QVBoxLayout(NULL);
          pElementsLayout->setSpacing(1);
          pElementsLayout->setContentsMargins(5, 0, 0, 0);
          pSubGroup->setInnerLayout(pElementsLayout);

          pLayout->addWidget(pSubGroup);
        }

        ezInt32 iElements = items.IsEmpty() ? 0 : 0x7FFFFFFF;

        for (const auto& item : items)
        {
          const auto& accessor = bEditorProperty ? item.m_pObject->GetEditorTypeAccessor() : item.m_pObject->GetTypeAccessor();

          iElements = ezMath::Min(iElements, accessor.GetCount(ParentPath));
        }

        EZ_ASSERT_DEV(iElements >= 0, "Mismatch between storage and RTTI (%i)", iElements);

        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          for (ezInt32 i = 0; i < iElements; ++i)
          {
            ezElementGroupBox* pSubGroup = new ezElementGroupBox((QWidget*)pElementsLayout->parent());

            QVBoxLayout* pSubLayout = new QVBoxLayout(NULL);
            pSubLayout->setContentsMargins(5, 0, 0, 0);
            pSubLayout->setSpacing(1);
            pSubGroup->setInnerLayout(pSubLayout);

            pElementsLayout->addWidget(pSubGroup);

            /// \todo read-only flag ?
            //pNewWidget->setEnabled(!pProp->GetFlags().IsSet(PropertyFlags::IsReadOnly));

            ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8> SubItems;


            for (const auto& item : items)
            {
              const auto& accessor = bEditorProperty ? item.m_pObject->GetEditorTypeAccessor() : item.m_pObject->GetTypeAccessor();

              ezUuid ObjectGuid = accessor.GetValue(ParentPath, i).ConvertTo<ezUuid>();

              ezPropertyEditorBaseWidget::Selection sel;
              sel.m_pObject = accessor.GetOwner()->GetDocumentObjectManager()->GetObject(ObjectGuid);
              //sel.m_Index = // supposed to be invalid;

              SubItems.PushBack(sel);
            }
            const ezRTTI* pSubtype = GetCommonBaseType(SubItems, bEditorProperty);

            pSubGroup->setTitle(QLatin1String("[") + QString::number(i) + QLatin1String("] - ") + QString::fromUtf8(pSubtype->GetTypeName()));

            pSubGroup->SetItems(SubItems);

            ezPropertyPath SubPath;
            BuildUI(SubItems, bEditorProperty, pSubtype, SubPath, pSubLayout);

          }
          ezAddSubElementButton* pSubElementButton = new ezAddSubElementButton("Add Item", pElementsLayout->parentWidget());
          pSubElementButton->Init(items, ParentPath, bEditorProperty);
          pElementsLayout->addWidget(pSubElementButton);
        }
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }

      }
      break;

    }


    ParentPath.PopBack();
  }
}

void ezRawPropertyWidget::ChangePropertyValue(const ezString& sPropertyPath, const ezVariant& value)
{
  auto it = m_PropertyWidgets.Find(sPropertyPath);

  if (it.IsValid())
  {
    //it.Value()->SetValue(value);
    /// \todo 8Bla!
  }
}

