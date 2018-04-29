#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Types/UniquePtr.h>

struct EZ_FOUNDATION_DLL ezSerializedBlock
{
  ezString m_Name;
  ezUniquePtr<ezAbstractObjectGraph> m_Graph;
};

class EZ_FOUNDATION_DLL ezAbstractGraphDdlSerializer
{
public:

  static void Write(ezStreamWriter& stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest);
  static ezResult Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void WriteDocument(ezStreamWriter& stream, const ezAbstractObjectGraph* pHeader, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypes, bool bCompactMode = true, ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest);
  static ezResult ReadDocument(ezStreamReader& stream, ezUniquePtr<ezAbstractObjectGraph>& pHeader, ezUniquePtr<ezAbstractObjectGraph>& pGraph, ezUniquePtr<ezAbstractObjectGraph>& pTypes, bool bApplyPatches = true);

  static ezResult ReadHeader(ezStreamReader& stream, ezAbstractObjectGraph* pGraph);

private:
  static ezResult ReadBlocks(ezStreamReader& stream, ezHybridArray<ezSerializedBlock, 3>& blocks);

};

