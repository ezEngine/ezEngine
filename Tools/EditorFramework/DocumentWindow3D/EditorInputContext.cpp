#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <QKeyEvent>

ezEditorInputContext* ezEditorInputContext::s_pActiveInputContext = nullptr;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorInputContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEditorInputContext::~ezEditorInputContext()
{
  if (s_pActiveInputContext == this)
    SetActiveInputContext(nullptr);
}

ezEditorInut ezEditorInputContext::keyPressEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->key() == Qt::Key_Escape)
  {
    FocusLost(true);
    SetActiveInputContext(nullptr);
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}
