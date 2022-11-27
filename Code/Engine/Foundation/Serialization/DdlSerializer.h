#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/UniquePtr.h>

class ezOpenDdlReaderElement;

struct EZ_FOUNDATION_DLL ezSerializedBlock
{
  ezString m_Name;
  ezUniquePtr<ezAbstractObjectGraph> m_Graph;
};

class EZ_FOUNDATION_DLL ezAbstractGraphDdlSerializer
{
public:
  static void Write(ezStreamWriter& inout_stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest);
  static ezResult Read(ezStreamReader& inout_stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void Write(ezOpenDdlWriter& inout_stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr);
  static ezResult Read(const ezOpenDdlReaderElement* pRootElement, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void WriteDocument(ezStreamWriter& inout_stream, const ezAbstractObjectGraph* pHeader, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypes, bool bCompactMode = true, ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest);
  static ezResult ReadDocument(ezStreamReader& inout_stream, ezUniquePtr<ezAbstractObjectGraph>& ref_pHeader, ezUniquePtr<ezAbstractObjectGraph>& ref_pGraph, ezUniquePtr<ezAbstractObjectGraph>& ref_pTypes, bool bApplyPatches = true);

  static ezResult ReadHeader(ezStreamReader& inout_stream, ezAbstractObjectGraph* pGraph);

private:
  static ezResult ReadBlocks(ezStreamReader& stream, ezHybridArray<ezSerializedBlock, 3>& blocks);
};
