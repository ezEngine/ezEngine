#pragma once

#include <ModelImporter/ImporterImplementation.h>

namespace fbxsdk
{
  class FbxManager;
  class FbxNode;
}

namespace ezModelImporter
{
  /// Importer implementation to import FBX files using the official FBX SDK.
  class EZ_MODELIMPORTER_DLL FBXSDKImporter : public ImporterImplementation
  {
  public:
    FBXSDKImporter();
    ~FBXSDKImporter();

    virtual ezArrayPtr<const ezString> GetSupportedFileFormats() const override;
    virtual ezSharedPtr<Scene> ImportScene(const char* szFileName, ezBitflags<ImportFlags> importFlags) override;

  private:

    ezDynamicArray<ezString> m_supportedFileFormats;
    fbxsdk::FbxManager* m_pFBXManager;
  };
}
