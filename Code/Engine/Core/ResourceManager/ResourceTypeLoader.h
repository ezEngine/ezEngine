#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Timestamp.h>

/// \brief Data returned by ezResourceTypeLoader implementations.
struct EZ_CORE_DLL ezResourceLoadData
{
  ezResourceLoadData()
  {
    m_pDataStream = nullptr;
    m_pCustomLoaderData = nullptr;
  }

  /// Used to keep track when the loaded file was modified last and thus when reloading of the resource might be necessary.
  ezTimestamp m_LoadedFileModificationDate;

  /// All loaded data should be stored in a memory stream. This stream reader allows the resource to read the memory stream.
  ezStreamReaderBase* m_pDataStream;

  /// Custom loader data, e.g. a pointer to a custom memory block, that needs to be freed when the resource is done updating.
  void* m_pCustomLoaderData;
};

/// \brief Base class for all resource loaders.
///
/// A resource loader handles preparing the data before the resource is updated with the data.
/// Resource loaders are always executed on a separate thread.
class EZ_CORE_DLL ezResourceTypeLoader
{
public:
  ezResourceTypeLoader() { }
  virtual ~ezResourceTypeLoader () { }

  /// \brief Override this function to implement the resource loading.
  ///
  /// This function should take the information from \a pResource, e.g. which file to load, and do the loading work.
  /// It should allocate temporary storage for the loaded data and encode it in a memory stream, such that the 
  /// resource can read all necessary information from the stream.
  ///
  /// \sa ezResourceLoadData
  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) = 0;

  /// \brief This function is called when the resource has been updated with the data from the resource loader and the loader can deallocate any temporary memory.
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) = 0;

  /// \brief If this function returns true, a resource is unloaded and loaded again to update its content.
  ///
  /// Call ezResourceBase::GetLoadedFileModificationTime() to query the file modification time that was returned 
  /// through ezResourceLoadData::m_LoadedFileModificationDate.
  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const { return false; }
};

/// \brief A default implementation of ezResourceTypeLoader for standard file loading.
///
/// The loader will interpret the ezResourceBase 'resource ID' as a path, read that full file into a memory stream.
/// The file modification data is stored as well.
/// Resources that use this loader can update their data as if they were reading the file directly.
class EZ_CORE_DLL ezResourceLoaderFromFile : public ezResourceTypeLoader
{
public:
  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override;
};

