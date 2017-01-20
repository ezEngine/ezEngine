#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyEventHandler.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <CoreUtils/Localization/TranslationLookup.h>


ezPropertyEventHandler::ezPropertyEventHandler()
  : m_bUndead(false)
  , m_pGrid(nullptr)
{
}

void ezPropertyEventHandler::Init(ezQtPropertyGridWidget* pGrid)
{
  m_pGrid = pGrid;
}

void ezPropertyEventHandler::PrepareToDie()
{
  m_bUndead = true;
}

void ezPropertyEventHandler::PropertyChangedHandler(const ezPropertyEvent& ed)
{
  if (m_bUndead)
    return;

  ezObjectAccessorBase* pObjectAccessor = m_pGrid->GetObjectAccessor();
  switch (ed.m_Type)
  {
  case  ezPropertyEvent::Type::SingleValueChanged:
    {
      ezStringBuilder sTemp; sTemp.Format("Change Property '{0}'", ezTranslate(ed.m_pProperty->GetPropertyName()));
      pObjectAccessor->StartTransaction(sTemp);

      ezStatus res;
      for (const auto& sel : *ed.m_pItems)
      {
        res = pObjectAccessor->SetValue(sel.m_pObject, ed.m_pProperty, ed.m_Value, sel.m_Index);
        if (res.m_Result.Failed())
          break;
      }

      if (res.m_Result.Failed())
        pObjectAccessor->CancelTransaction();
      else
        pObjectAccessor->FinishTransaction();

      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Changing the property failed.");

    }
    break;

  case  ezPropertyEvent::Type::BeginTemporary:
    {
      ezStringBuilder sTemp; sTemp.Format("Change Property '{0}'", ezTranslate(ed.m_pProperty->GetPropertyName()));
      pObjectAccessor->BeginTemporaryCommands(sTemp);
    }
    break;

  case  ezPropertyEvent::Type::EndTemporary:
    {
      pObjectAccessor->FinishTemporaryCommands();
    }
    break;

  case  ezPropertyEvent::Type::CancelTemporary:
    {
      pObjectAccessor->CancelTemporaryCommands();
    }
    break;
  }
}
