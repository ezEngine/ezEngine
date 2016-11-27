#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONWriter.h>

class ezStreamWriter;
class ezStreamReader;
class ezAbstractObjectGraph;

class EZ_FOUNDATION_DLL ezAbstractGraphJsonSerializer
{
public:
  static void Write(ezStreamWriter& stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr, ezStandardJSONWriter::WhitespaceMode mode = ezStandardJSONWriter::WhitespaceMode::None); // [tested]
  static ezResult Read(ezStreamReader& stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true); // [tested]
};
