#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/ExpressionByteCode.h>

ezExpressionByteCode::ezExpressionByteCode()
  : m_uiNumInputRegisters(0)
  , m_uiNumTempRegisters(0)
{

}

ezExpressionByteCode::~ezExpressionByteCode()
{

}

void ezExpressionByteCode::Disassemble(ezStringBuilder& out_sDisassembly) const
{
  out_sDisassembly.Append("Dummy");
}
