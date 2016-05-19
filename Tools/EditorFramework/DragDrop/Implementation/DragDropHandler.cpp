#include <PCH.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragDropHandler, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDragDropHandler* ezDragDropHandler::s_pActiveDnD = nullptr;

ezDragDropHandler::ezDragDropHandler()
{

}


bool ezDragDropHandler::BeginDragDropOperation(const ezDragDropInfo* pInfo)
{
  EZ_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  for (ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (pRtti->IsDerivedFrom<ezDragDropHandler>() && pRtti->GetAllocator()->CanAllocate())
    {
      ezDragDropHandler* pDnD = static_cast<ezDragDropHandler*>(pRtti->GetAllocator()->Allocate());

      if (pDnD->CanHandle(pInfo))
      {
        s_pActiveDnD = pDnD;

        s_pActiveDnD->OnDragBegin(pInfo);
        return true;
      }

      pRtti->GetAllocator()->Deallocate(pDnD);
    }
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

  s_pActiveDnD->CancelDragDrop();

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

