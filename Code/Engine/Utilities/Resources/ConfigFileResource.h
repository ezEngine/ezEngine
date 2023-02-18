#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DependencyFile.h>
#include <Foundation/Strings/HashedString.h>
#include <Utilities/UtilitiesDLL.h>

using ezConfigFileResourceHandle = ezTypedResourceHandle<class ezConfigFileResource>;

/// \brief This resource loads config files containing key/value pairs
///
/// The config files usually use the file extension '.ezConfig'.
///
/// The file format looks like this:
///
/// To declare a key/value pair for the first time, write its type, name and value:
///   int i = 1
///   float f = 2.3
///   bool b = false
///   string s = "hello"
///
/// To set a variable to a different value than before, it has to be marked with 'override':
///
///   override i = 4
///
/// The format supports C preprocessor features like #include, #define, #ifdef, etc.
/// This can be used to build hierarchical config files:
///
///   #include "BaseConfig.ezConfig"
///   override int SomeValue = 7
///
/// It can also be used to define 'enum types':
///
///   #define SmallValue 3
///   #define BigValue 5
///   int MyValue = BigValue
///
/// Since resources can be reloaded at runtime, config resources are a convenient way to define game parameters
/// that you may want to tweak at any time.
/// Using C preprocessor logic (#define, #if, #else, etc) you can quickly select between different configuration sets.
///
/// Once loaded, accessing the data is very efficient.
class EZ_UTILITIES_DLL ezConfigFileResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConfigFileResource, ezResource);

  EZ_RESOURCE_DECLARE_COMMON_CODE(ezConfigFileResource);

public:
  ezConfigFileResource();
  ~ezConfigFileResource();

  /// \brief Returns the 'int' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  ezInt32 GetInt(ezTempHashedString sName) const;

  /// \brief Returns the 'float' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  float GetFloat(ezTempHashedString sName) const;

  /// \brief Returns the 'bool' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  bool GetBool(ezTempHashedString sName) const;

  /// \brief Returns the 'string' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  const char* GetString(ezTempHashedString sName) const;

  /// \brief Returns the 'int' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  ezInt32 GetInt(ezTempHashedString sName, ezInt32 iFallback) const;

  /// \brief Returns the 'float' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  float GetFloat(ezTempHashedString sName, float fFallback) const;

  /// \brief Returns the 'bool' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  bool GetBool(ezTempHashedString sName, bool bFallback) const;

  /// \brief Returns the 'string' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  const char* GetString(ezTempHashedString sName, const char* szFallback) const;

protected:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  friend class ezConfigFileResourceLoader;

  ezHashTable<ezHashedString, ezInt32> m_IntData;
  ezHashTable<ezHashedString, float> m_FloatData;
  ezHashTable<ezHashedString, ezString> m_StringData;
  ezHashTable<ezHashedString, bool> m_BoolData;

  ezDependencyFile m_RequiredFiles;
};


class EZ_UTILITIES_DLL ezConfigFileResourceLoader : public ezResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    ezDefaultMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
    ezDependencyFile m_RequiredFiles;

    ezResult PrePropFileLocator(const char* szCurAbsoluteFile, const char* szIncludeFile, ezPreprocessor::IncludeType incType, ezStringBuilder& out_sAbsoluteFilePath);
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};
