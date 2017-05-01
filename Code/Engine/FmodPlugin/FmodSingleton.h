#pragma once

#include <FmodPlugin/Basics.h>
#include <FmodPlugin/PluginInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>

struct ezGameApplicationEvent;

class EZ_FMODPLUGIN_DLL ezFmod : public ezFmodInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezFmod, ezFmodInterface);

public:
  ezFmod();

  void Startup();
  void Shutdown();

  virtual void SetNumListeners(ezUInt8 uiNumListeners) override;
  virtual ezUInt8 GetNumListeners() override;
  virtual void UpdateSound() override;

  FMOD::Studio::System* GetSystem() const { return m_pFmodSystem; }
  FMOD::System* GetLowLevelSystem() const { return m_pLowLevelSystem; }

  static void GameApplicationEventHandler(const ezGameApplicationEvent& e);

  /// \brief Configures how many reverb ('EAX') volumes are being blended/mixed for a sound.
  ///
  /// The number is clamped between 0 and 4. 0 Means all environmental effects are disabled for all sound sources.
  /// 1 means only the most important reverb is applied. 2, 3 and 4 allow to add more fidelity, but will cost more CPU resources.
  ///
  /// The default is currently 4.
  void SetNumBlendedReverbVolumes(ezUInt8 uiNumBlendedVolumes);

  /// \brief See SetNumBlendedReverbVolumes()
  ezUInt8 GetNumBlendedReverbVolumes() const { return m_uiNumBlendedVolumes; }

private:
  bool m_bInitialized;
  ezUInt8 m_uiNumBlendedVolumes = 4;

  FMOD::Studio::System* m_pFmodSystem;
  FMOD::System* m_pLowLevelSystem;
  
};

EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_FMODPLUGIN_DLL, ezFmodPlugin);