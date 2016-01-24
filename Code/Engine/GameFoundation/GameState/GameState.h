#pragma once

#include <GameFoundation/Basics.h>
#include <GameFoundation/Declarations.h>

#include <Foundation/Reflection/Reflection.h>

enum class ezGameStateCanHandleThis
{
  No,
  Yes,
  AsFallback,
};


class EZ_GAMEFOUNDATION_DLL ezGameState : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameState, ezReflectedClass)

protected:
  ezGameState();

public:
  virtual ~ezGameState();

  virtual void ProcessInput() { }

  virtual void Activate() { }
  virtual void Deactivate() { }
  
  virtual void BeforeWorldUpdate() { }
  virtual void AfterWorldUpdate() { }

  EZ_FORCE_INLINE ezGameApplication* GetApplication() const
  {
    return m_pApplication;
  }

  virtual ezGameStateCanHandleThis CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const = 0;

private:
  friend class ezGameApplication;
  ezGameApplication* m_pApplication;
};
