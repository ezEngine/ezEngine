#pragma once

#include <GameFoundation/Basics.h>

#include <Foundation/Reflection/Reflection.h>

class ezGameApplication;

class EZ_GAMEFOUNDATION_DLL ezGameState : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameState, ezReflectedClass)

protected:
  ezGameState() : m_pApplication(nullptr) { }

public:
  virtual ~ezGameState() { }

  virtual void Activate() { }
  virtual void Deactivate() { }
  
  virtual void BeforeWorldUpdate() { }
  virtual void AfterWorldUpdate() { }

  EZ_FORCE_INLINE ezGameApplication* GetApplication() const
  {
    return m_pApplication;
  }

private:
  friend class ezGameApplication;
  ezGameApplication* m_pApplication;
};
