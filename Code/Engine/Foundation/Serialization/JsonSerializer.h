#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/IO/JSONWriter.h>

class EZ_FOUNDATION_DLL ezAbstractGraphJsonSerializer
{
public:

  static void Write(ezStreamWriterBase& stream, const ezAbstractObjectGraph* pGraph, ezStandardJSONWriter::WhitespaceMode mode = ezStandardJSONWriter::WhitespaceMode::None);
  static void Read(ezStreamReaderBase& stream, ezAbstractObjectGraph* pGraph);

private:

};