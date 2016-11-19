#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/OpenDdlWriter.h>

class EZ_FOUNDATION_DLL ezAbstractGraphDdlSerializer
{
public:

  static void Write(ezStreamWriter& stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr, ezOpenDdlWriter::WhitespaceMode mode = ezOpenDdlWriter::WhitespaceMode::None);
  static ezResult Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr);

private:

};
