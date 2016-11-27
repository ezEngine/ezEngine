#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/IO/OpenDdlWriter.h>

class EZ_FOUNDATION_DLL ezAbstractGraphDdlSerializer
{
public:

  static void Write(ezStreamWriter& stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest);
  static ezResult Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);
  static ezResult ReadHeader(ezStreamReader& stream, ezAbstractObjectGraph* pGraph);

private:

};
