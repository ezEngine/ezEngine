#pragma once

#include <FmodPlugin/Basics.h>
#include <FmodPlugin/PluginInterface.h>
#include <Foundation/Configuration/Plugin.h>

class EZ_FMODPLUGIN_DLL ezFmod : public ezFmodInterface
{
public:
  ezFmod();

  static ezFmod* GetSingleton();

  void Startup();
  void Shutdown();

  virtual void SetNumListeners(ezUInt8 uiNumListeners) override;
  virtual ezUInt8 GetNumListeners() override;

  FMOD::Studio::System* GetSystem() const { return m_pFmodSystem; }
  FMOD::System* GetLowLevelSystem() const { return m_pLowLevelSystem; }

private:
  bool m_bInitialized;

  FMOD::Studio::System* m_pFmodSystem;
  FMOD::System* m_pLowLevelSystem;
  
};

EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_FMODPLUGIN_DLL, ezFmodPlugin);