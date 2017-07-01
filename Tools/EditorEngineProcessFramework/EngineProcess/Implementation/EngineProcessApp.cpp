#include <PCH.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

EZ_IMPLEMENT_SINGLETON(ezEditorEngineProcessApp);

static ezEditorEngineProcessApp g_Instance;

ezEditorEngineProcessApp::ezEditorEngineProcessApp()
  : m_SingletonRegistrar(this)
{

}

ezEditorEngineProcessApp::~ezEditorEngineProcessApp()
{

}

