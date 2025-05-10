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
};

class EZ_EDITORFRAMEWORK_DLL ezFreeAllResourcesMsg : public ezProcessMessage
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFreeAllResourcesMsg, ezProcessMessage);
};
