#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Core/Actor/ActorManager.h>

class ezActorEditorWnd;
struct ezWindowCreationDesc;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezActorManagerEditorWnd : public ezActorManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorManagerEditorWnd, ezActorManager);

public:
  ezActorManagerEditorWnd();
  ~ezActorManagerEditorWnd();

  ezActorEditorWnd* CreateEditorWndActor(const char* szActorName, const char* szGroupName, ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);
  void DestroyEditorWndActor(ezActorEditorWnd* pActor);
};
