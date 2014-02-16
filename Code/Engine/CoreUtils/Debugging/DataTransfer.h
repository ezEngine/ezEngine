#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Containers/Set.h>

class EZ_COREUTILS_DLL ezDataTransfer;

class EZ_COREUTILS_DLL ezDataTransferObject
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataTransferObject);

public:
  ezDataTransferObject(const ezDataTransfer& BelongsTo, const char* szObjectName, const char* szMimeType);

  ezStreamWriterBase& GetWriter() { return m_Msg.GetWriter(); }

private:
  friend class ezDataTransfer;

  ezTelemetryMessage m_Msg;
};

class EZ_COREUTILS_DLL ezDataTransfer
{
public:
  ezDataTransfer();
  ~ezDataTransfer();
  
  void DisableDataTransfer();
  void EnableDataTransfer(const char* szDataName);

  void RequestDataTransfer();

  bool IsTransferRequested(bool bReset = true);

  void Transfer(ezDataTransferObject& Object);

private:
  virtual void OnTransferRequest() { }

  void SendStatus();

private:
  friend class ezDataTransferObject;

  static void TelemetryMessage(void* pPassThrough);
  static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e);
  static void Initialize();
  static void SendAllDataTransfers();

  static bool s_bInitialized;

  bool m_bEnabled;
  bool m_bTransferRequested;
  ezString m_sDataName;
  static ezSet<ezDataTransfer*> s_AllTransfers;
};