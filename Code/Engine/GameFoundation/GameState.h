#pragma once

#include <GameFoundation/Basics.h>

#include <Foundation/Reflection/Reflection.h>

class ezGameApplication;

class EZ_GAMEFOUNDATION_DLL ezGameStateBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameStateBase)

public:
  ezGameStateBase() : m_pApplication(nullptr) { }
  virtual ~ezGameStateBase() { }

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
