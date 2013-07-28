#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Profiling/Profiling.h>

#include <Core/Application/Application.h>
#include <Core/World/World.h>

#include "DamageComponent.h"
#include "HealthComponent.h"


class SampleGameApp : public ezApplication
{
public:
  virtual void AfterEngineInit() EZ_OVERRIDE
  {
    ezLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    m_pWorld = EZ_DEFAULT_NEW(ezWorld)("Test");

    ezGameObjectHandle handle;
    ezGameObject* pObject;

    {
      ezGameObjectDesc desc;
      handle = m_pWorld->CreateObject(desc);
      pObject = m_pWorld->GetObject(handle);
    }

    {
      HealthComponentManager* pManager = m_pWorld->CreateComponentManager<HealthComponentManager>();

      ezComponentHandle healthComponent = pManager->CreateComponent();

      HealthComponent* pHealthComponent = pManager->GetComponent(healthComponent);
      pHealthComponent->SetHealth(80.0f);

      pObject->AddComponent(healthComponent);
    }

    {
      pObject->AddComponent(m_pWorld->CreateComponentManager<DamageComponentManager>()->CreateComponent());
    }
  }

  virtual void BeforeEngineShutdown() EZ_OVERRIDE
  {
    EZ_DEFAULT_DELETE(m_pWorld);
  }

  virtual ezApplication::ApplicationExecution Run() EZ_OVERRIDE
  {
    m_pWorld->Update();

    if (m_pWorld->GetObjectCount() > 0)
      return ezApplication::Continue;

    return ezApplication::Quit;
  }

private:
  ezWorld* m_pWorld;
};

EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);
