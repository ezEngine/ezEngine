#pragma once

#include <ProceduralPlacementPlugin/Basics.h>

class ezExpressionAST;
class ezExpressionByteCode;

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezExpressionCompiler
{
public:
  ezExpressionCompiler();
  ~ezExpressionCompiler();

  ezResult Compile(ezExpressionAST& ast, ezExpressionByteCode& out_byteCode);

private:

};

