#pragma once

#include <EditorEngineProcess/EngineProcGameApp.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

class ezEditorEngineProcessAppUWP;

class ezEngineProcessGameApplicationUWP : public ezEngineProcessGameApplication
{
public:
  ezEngineProcessGameApplicationUWP();
  ~ezEngineProcessGameApplicationUWP();

protected:
  virtual void ProcessApplicationInput() override;
  virtual void DoConfigureInput(bool bReinitialize) override;
  virtual ezUniquePtr<ezEditorEngineProcessApp> CreateEngineProcessApp() override;

private:
  ezEditorEngineProcessAppUWP* m_pEngineProcessApp;
  ezTime m_HandPressTime;
  ezVec3 m_vHandStartPosition;
};

#endif
