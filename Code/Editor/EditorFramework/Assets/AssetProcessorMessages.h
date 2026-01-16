#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>

class EZ_EDITORFRAMEWORK_DLL ezProcessAssetMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessAssetMsg, ezProcessMessage);

public:
  ezUuid m_AssetGuid;
  ezUInt64 m_AssetHash = 0;
  ezUInt64 m_ThumbHash = 0;
  ezUInt64 m_PackageHash = 0;
  ezString m_sAssetPath;
  ezString m_sPlatform;
  ezDynamicArray<ezString> m_DepRefHull;
};

class EZ_EDITORFRAMEWORK_DLL ezProcessAssetResponseMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessAssetResponseMsg, ezProcessMessage);

public:
  ezTransformStatus m_Status;
  mutable ezDynamicArray<ezLogEntry> m_LogEntries;

  // If the fields below are used, the ezEditorProcessor detected a hash missmatch between the editor and its own state. It will send over its own state so the editor can detect differences. See ezEditorProcessorProcess::HandleHashMissmatch.
  mutable ezMap<ezString, ezUInt64> m_MissmatchTransformDependencies; ///< Hashes of all transform dependencies.
  mutable ezMap<ezString, ezUInt64> m_MissmatchThumbnailDependencies; ///< Hashes of all thumbnail dependencies.
  ezUInt64 m_uiMissmatchAssetHash = 0;                                ///< Transform hash observed by the ezEditorProcessor.
  ezUInt64 m_uiMissmatchThumbHash = 0;                                ///< Thumbnail hash observed by the ezEditorProcessor.
};

class EZ_EDITORFRAMEWORK_DLL ezFreeAllResourcesMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFreeAllResourcesMsg, ezProcessMessage);
};
