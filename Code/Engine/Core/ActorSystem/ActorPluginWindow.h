#pragma once

#include <Core/ActorSystem/ActorPlugin.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/System/Window.h>

class ezActor;
class ezWindowOutputTargetBase;
class ezWindowBase;

/// Actor plugin providing window management functionality.
///
/// Base class for actor plugins that need to manage a window and output target.
/// Provides the interface for accessing window resources required by actors.
class EZ_CORE_DLL ezActorPluginWindow : public ezActorPlugin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorPluginWindow, ezActorPlugin);

public:
  /// Returns the window managed by this plugin.
  virtual ezWindowBase* GetWindow() const = 0;

  /// Returns the output target for rendering to the window.
  virtual ezWindowOutputTargetBase* GetOutputTarget() const = 0;

protected:
  virtual void Update() override;
};

/// Actor plugin that owns its window and output target resources.
///
/// This plugin creates and manages its own window and output target,
/// taking full ownership and responsibility for their lifetime.
class EZ_CORE_DLL ezActorPluginWindowOwner : public ezActorPluginWindow
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorPluginWindowOwner, ezActorPluginWindow);

public:
  virtual ~ezActorPluginWindowOwner();
  virtual ezWindowBase* GetWindow() const override;
  virtual ezWindowOutputTargetBase* GetOutputTarget() const override;

  ezUniquePtr<ezWindowBase> m_pWindow;
  ezUniquePtr<ezWindowOutputTargetBase> m_pWindowOutputTarget;
};

