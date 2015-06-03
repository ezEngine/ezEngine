#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

ezEditorInputContext* ezEditorInputContext::s_pActiveInputContext = nullptr;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorInputContext, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEditorInputContext::~ezEditorInputContext()
{
  if (s_pActiveInputContext == this)
    SetActiveInputContext(nullptr);
}