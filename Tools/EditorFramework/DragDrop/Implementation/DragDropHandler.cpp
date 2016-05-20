#include <PCH.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragDropHandler, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDragDropHandler* ezDragDropHandler::s_pActiveDnD = nullptr;

ezDragDropHandler::ezDragDropHandler()
{

}


bool ezDragDropHandler::BeginDragDropOperation(const ezDragDropInfo* pInfo, ezDragDropConfig* pConfigToFillOut)
{
  EZ_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  float fBestValue = 0.0f;
  ezDragDropHandler* pBestDnD = nullptr;

  for (ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (pRtti->IsDerivedFrom<ezDragDropHandler>() && pRtti->GetAllocator()->CanAllocate())
    {
      ezDragDropHandler* pDnD = static_cast<ezDragDropHandler*>(pRtti->GetAllocator()->Allocate());

      const float fValue = pDnD->CanHandle(pInfo);
      if (fValue > fBestValue)
      {
        if (pBestDnD != nullptr)
        {
          pBestDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pBestDnD);
        }

        fBestValue = fValue;
        pBestDnD = pDnD;
      }
      else
      {
        pDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pDnD);
      }
    }
  }

  if (pBestDnD != nullptr)
  {
    if (pConfigToFillOut != nullptr)
      pBestDnD->RequestConfiguration(pConfigToFillOut);

    s_pActiveDnD = pBestDnD;
    s_pActiveDnD->OnDragBegin(pInfo);
    return true;
  }

  return false;
}

void ezDragDropHandler::UpdateDragDropOperation(const ezDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragUpdate(pInfo);
}

void ezDragDropHandler::FinishDragDrop(const ezDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDrop(pInfo);

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

void ezDragDropHandler::CancelDragDrop()
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragCancel();

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

