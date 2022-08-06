#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Core/ResourceManager/Resource.h>

using ezAudioControlCollectionResourceHandle = ezTypedResourceHandle<class ezAudioControlCollectionResource>;

/// \brief Represents one audio control, used by a single audio middleware.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioControlCollectionEntry
{
  ezString m_sName;                                      ///< Optional, can be used to lookup the resource at runtime with a nice name. E.g. "SkyTexture" instead of some GUID.
  ezString m_sControlFile;                               ///< The path to the audio system control.
  ezDefaultMemoryStreamStorage* m_pControlBufferStorage; ///< Buffer storage that contains the control data. Only have a value for loaded resources.
  ezEnum<ezAudioSystemControlType> m_Type;               ///< The type of the control.
};

/// \brief Describes a full ezAudioControlCollectionResource, ie. lists all the controls that the collection contains.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioControlCollectionResourceDescriptor
{
  ezDynamicArray<ezAudioControlCollectionEntry> m_Entries;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

using ezCollectionResourceHandle = ezTypedResourceHandle<class ezCollectionResource>;

/// \brief An ezAudioControlCollectionResource is used by the audio system to map a collection of audio controls to a single audio middleware.
///
/// For each audio control should specify the control name, the control type, and the path to the generated middleware control file.
/// Those controls will now be able to be used by the audio system through components.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioControlCollectionResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioControlCollectionResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezAudioControlCollectionResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezAudioControlCollectionResource, ezAudioControlCollectionResourceDescriptor);

public:
  ezAudioControlCollectionResource();

  /// \brief Registers this collection to the audio system.
  ///
  /// \note This is called automatically at initialization by the audio system on the control collection
  /// asset having the same name than the current audio middleware.
  ///
  /// Calling this twice has no effect.
  void Register();

  /// \brief Removes the registered controls from the audio system.
  ///
  /// Calling this twice has no effect.
  void Unregister();

  /// \brief Returns the resource descriptor for this resource.
  const ezAudioControlCollectionResourceDescriptor& GetDescriptor() const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  bool m_bRegistered = false;
  ezAudioControlCollectionResourceDescriptor m_Collection;
};
