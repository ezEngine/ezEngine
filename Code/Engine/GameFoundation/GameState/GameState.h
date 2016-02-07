#pragma once

#include <GameFoundation/Basics.h>
#include <GameFoundation/Declarations.h>

#include <Foundation/Reflection/Reflection.h>
#include <RendererFoundation/Basics.h>
#include <CoreUtils/Graphics/Camera.h>

class ezWindow;
class ezView;

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

  virtual void Activate(ezGameApplicationType AppType, ezWorld* pWorld);
  virtual void Deactivate();
  
  virtual void BeforeWorldUpdate() { }
  virtual void AfterWorldUpdate() { }

  EZ_FORCE_INLINE ezGameApplication* GetApplication() const { return m_pApplication; }

  virtual ezGameStateCanHandleThis CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const = 0;

protected:
  /// \brief Creates a default window (ezGameStateWindow) adds it to the application and fills out m_pMainWindow and m_hMainSwapChain
  virtual void CreateMainWindow();

  /// \brief Destroys the m_pMainWindow.
  /// Unless overridden Deactivate() will call this.
  virtual void DestroyMainWindow();

  /// \brief Configures available input devices, e.g. sets mouse speed, cursor clipping, etc.
  /// Unless overridden Activate() will call this.
  virtual void ConfigureInputDevices();

  /// \brief Adds custom input actions, if necessary.
  /// Unless overridden Activate() will call this.
  virtual void ConfigureInputActions();

  /// \brief Creates a default render pipeline. Unless overridden, Activate() will do this for the main window.
  virtual void CreateMainRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV);

  /// \brief Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  void ChangeMainWorld(ezWorld* pNewMainWorld);

  /// \brief Sets up m_MainCamera for first use
  virtual void ConfigureMainCamera();

  ezWindow* m_pMainWindow;
  ezGALSwapChainHandle m_hMainSwapChain;
  ezWorld* m_pMainWorld;
  ezView* m_pMainView;
  ezCamera m_MainCamera;

private:
  friend class ezGameApplication;
  ezGameApplication* m_pApplication;
};
