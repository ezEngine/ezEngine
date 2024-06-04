#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  struct ExecutionContext
  {
    ezExpression::Register* m_pRegisters = nullptr;
    ezUInt32 m_uiNumInstances = 0;
    ezUInt32 m_uiNumSimd4Instances = 0;
    ezArrayPtr<const ezProcessingStream*> m_Inputs;
    ezArrayPtr<ezProcessingStream*> m_Outputs;
    ezArrayPtr<const ezExpressionFunction*> m_Functions;
    const ezExpression::GlobalData* m_pGlobalData = nullptr;
  };

  using ByteCodeType = ezExpressionByteCode::StorageType;
  using OpFunc = void (*)(const ByteCodeType*& pByteCode, ExecutionContext& context);

#define DEFINE_TARGET_REGISTER()                                                                                                        \
  ezExpression::Register* r = context.m_pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode) * context.m_uiNumSimd4Instances; \
  ezExpression::Register* re = r + context.m_uiNumSimd4Instances;                                                                       \
  EZ_IGNORE_UNUSED(re);

#define DEFINE_OP_REGISTER(name) \
  const ezExpression::Register* name = context.m_pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode) * context.m_uiNumSimd4Instances;

#define DEFINE_CONSTANT(name)                                                      \
  const ezUInt32 EZ_PP_CONCAT(name, Raw) = *pByteCode;                                \
  EZ_IGNORE_UNUSED(EZ_PP_CONCAT(name, Raw));                                          \
  const ezExpression::Register tmp = ezExpressionByteCode::GetConstant(pByteCode); \
  const ezExpression::Register* name = &tmp;

#define UNARY_OP_INNER_LOOP(code) \
  code;                           \
  ++r;                            \
  ++a;

#define DEFINE_UNARY_OP(name, code)                                                   \
  void EZ_PP_CONCAT(name, _4)(const ByteCodeType*& pByteCode, ExecutionContext& context) \
  {                                                                                   \
    DEFINE_TARGET_REGISTER();                                                         \
    DEFINE_OP_REGISTER(a);                                                            \
    while (r != re)                                                                   \
    {                                                                                 \
      UNARY_OP_INNER_LOOP(code)                                                       \
    }                                                                                 \
  }

#define BINARY_OP_INNER_LOOP(code)        \
  code;                                   \
  ++r;                                    \
  ++a;                                    \
  if constexpr (RightIsConstant == false) \
  {                                       \
    ++b;                                  \
  }

#define DEFINE_BINARY_OP(name, code)                                                                                \
  template <bool RightIsConstant>                                                                                   \
  void EZ_PP_CONCAT(name, _4)(const ByteCodeType*& pByteCode, ExecutionContext& context)                               \
  {                                                                                                                 \
    DEFINE_TARGET_REGISTER();                                                                                       \
    DEFINE_OP_REGISTER(a);                                                                                          \
    ezUInt32 bRaw;                                                                                                  \
    EZ_IGNORE_UNUSED(bRaw);                                                                                         \
    ezExpression::Register bConstant;                                                                               \
    const ezExpression::Register* b;                                                                                \
    EZ_IGNORE_UNUSED(b);                                                                                            \
    if constexpr (RightIsConstant)                                                                                  \
    {                                                                                                               \
      bRaw = *pByteCode;                                                                                            \
      bConstant = ezExpressionByteCode::GetConstant(pByteCode);                                                     \
      b = &bConstant;                                                                                               \
    }                                                                                                               \
    else                                                                                                            \
    {                                                                                                               \
      b = context.m_pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode) * context.m_uiNumSimd4Instances; \
    }                                                                                                               \
    while (r != re)                                                                                                 \
    {                                                                                                               \
      BINARY_OP_INNER_LOOP(code)                                                                                    \
    }                                                                                                               \
  }

#define TERNARY_OP_INNER_LOOP(code) \
  code;                             \
  ++r;                              \
  ++a;                              \
  ++b;                              \
  ++c;

#define DEFINE_TERNARY_OP(name, code)                                                 \
  void EZ_PP_CONCAT(name, _4)(const ByteCodeType*& pByteCode, ExecutionContext& context) \
  {                                                                                   \
    DEFINE_TARGET_REGISTER();                                                         \
    DEFINE_OP_REGISTER(a);                                                            \
    DEFINE_OP_REGISTER(b);                                                            \
    DEFINE_OP_REGISTER(c);                                                            \
    while (r != re)                                                                   \
    {                                                                                 \
      TERNARY_OP_INNER_LOOP(code)                                                     \
    }                                                                                 \
  }

  DEFINE_UNARY_OP(AbsF, r->f = a->f.Abs());
  DEFINE_UNARY_OP(AbsI, r->i = a->i.Abs());
  DEFINE_UNARY_OP(SqrtF, r->f = a->f.GetSqrt());

  DEFINE_UNARY_OP(ExpF, r->f = ezSimdMath::Exp(a->f));
  DEFINE_UNARY_OP(LnF, r->f = ezSimdMath::Ln(a->f));
  DEFINE_UNARY_OP(Log2F, r->f = ezSimdMath::Log2(a->f));
  DEFINE_UNARY_OP(Log2I, r->i = ezSimdMath::Log2i(a->i));
  DEFINE_UNARY_OP(Log10F, r->f = ezSimdMath::Log10(a->f));
  DEFINE_UNARY_OP(Pow2F, r->f = ezSimdMath::Pow2(a->f));

  DEFINE_UNARY_OP(SinF, r->f = ezSimdMath::Sin(a->f));
  DEFINE_UNARY_OP(CosF, r->f = ezSimdMath::Cos(a->f));
  DEFINE_UNARY_OP(TanF, r->f = ezSimdMath::Tan(a->f));

  DEFINE_UNARY_OP(ASinF, r->f = ezSimdMath::ASin(a->f));
  DEFINE_UNARY_OP(ACosF, r->f = ezSimdMath::ACos(a->f));
  DEFINE_UNARY_OP(ATanF, r->f = ezSimdMath::ATan(a->f));

  DEFINE_UNARY_OP(RoundF, r->f = a->f.Round());
  DEFINE_UNARY_OP(FloorF, r->f = a->f.Floor());
  DEFINE_UNARY_OP(CeilF, r->f = a->f.Ceil());
  DEFINE_UNARY_OP(TruncF, r->f = a->f.Trunc());

  DEFINE_UNARY_OP(NotI, r->i = ~a->i);
  DEFINE_UNARY_OP(NotB, r->b = !a->b);

  DEFINE_UNARY_OP(IToF, r->f = a->i.ToFloat());
  DEFINE_UNARY_OP(FToI, r->i = ezSimdVec4i::Truncate(a->f));

  DEFINE_BINARY_OP(AddF, r->f = a->f + b->f);
  DEFINE_BINARY_OP(AddI, r->i = a->i + b->i);

  DEFINE_BINARY_OP(SubF, r->f = a->f - b->f);
  DEFINE_BINARY_OP(SubI, r->i = a->i - b->i);

  DEFINE_BINARY_OP(MulF, r->f = a->f.CompMul(b->f));
  DEFINE_BINARY_OP(MulI, r->i = a->i.CompMul(b->i));

  DEFINE_BINARY_OP(DivF, r->f = a->f.CompDiv(b->f));
  DEFINE_BINARY_OP(DivI, r->i = a->i.CompDiv(b->i));

  DEFINE_BINARY_OP(MinF, r->f = a->f.CompMin(b->f));
  DEFINE_BINARY_OP(MinI, r->i = a->i.CompMin(b->i));

  DEFINE_BINARY_OP(MaxF, r->f = a->f.CompMax(b->f));
  DEFINE_BINARY_OP(MaxI, r->i = a->i.CompMax(b->i));

  DEFINE_BINARY_OP(ShlI, r->i = a->i << b->i);
  DEFINE_BINARY_OP(ShrI, r->i = a->i >> b->i);
  DEFINE_BINARY_OP(ShlI_C, r->i = a->i << bRaw);
  DEFINE_BINARY_OP(ShrI_C, r->i = a->i >> bRaw);
  DEFINE_BINARY_OP(AndI, r->i = a->i & b->i);
  DEFINE_BINARY_OP(XorI, r->i = a->i ^ b->i);
  DEFINE_BINARY_OP(OrI, r->i = a->i | b->i);

  DEFINE_BINARY_OP(EqF, r->b = a->f == b->f);
  DEFINE_BINARY_OP(EqI, r->b = a->i == b->i);
  DEFINE_BINARY_OP(EqB, r->b = a->b == b->b);

  DEFINE_BINARY_OP(NEqF, r->b = a->f != b->f);
  DEFINE_BINARY_OP(NEqI, r->b = a->i != b->i);
  DEFINE_BINARY_OP(NEqB, r->b = a->b != b->b);

  DEFINE_BINARY_OP(LtF, r->b = a->f < b->f);
  DEFINE_BINARY_OP(LtI, r->b = a->i < b->i);

  DEFINE_BINARY_OP(LEqF, r->b = a->f <= b->f);
  DEFINE_BINARY_OP(LEqI, r->b = a->i <= b->i);

  DEFINE_BINARY_OP(GtF, r->b = a->f > b->f);
  DEFINE_BINARY_OP(GtI, r->b = a->i > b->i);

  DEFINE_BINARY_OP(GEqF, r->b = a->f >= b->f);
  DEFINE_BINARY_OP(GEqI, r->b = a->i >= b->i);

  DEFINE_BINARY_OP(AndB, r->b = a->b && b->b);
  DEFINE_BINARY_OP(OrB, r->b = a->b || b->b);

  DEFINE_TERNARY_OP(SelF, r->f = ezSimdVec4f::Select(a->b, b->f, c->f));
  DEFINE_TERNARY_OP(SelI, r->i = ezSimdVec4i::Select(a->b, b->i, c->i));
  DEFINE_TERNARY_OP(SelB, r->b = ezSimdVec4b::Select(a->b, b->b, c->b));

  void VM_MovX_R_4(const ByteCodeType*& pByteCode, ExecutionContext& context)
  {
    DEFINE_TARGET_REGISTER();
    DEFINE_OP_REGISTER(a);
    while (r != re)
    {
      r->i = a->i;
      ++r;
      ++a;
    }
  }

  void VM_MovX_C_4(const ByteCodeType*& pByteCode, ExecutionContext& context)
  {
    EZ_WARNING_PUSH()
    EZ_WARNING_DISABLE_MSVC(4189)

    DEFINE_TARGET_REGISTER();
    DEFINE_CONSTANT(a);
    while (r != re)
    {
      r->i = a->i;
      ++r;
    }

    EZ_WARNING_POP()
  }

  template <typename ValueType, typename StreamType>
  EZ_ALWAYS_INLINE ValueType ReadInputData(const ezUInt8*& ref_pData, ezUInt32 uiStride)
  {
    ValueType value = *reinterpret_cast<const StreamType*>(ref_pData);
    ref_pData += uiStride;
    return value;
  }

  template <typename RegisterType, typename ValueType, typename StreamType>
  void LoadInput(RegisterType* r, RegisterType* pRe, const ezProcessingStream& input, ezUInt32 uiNumRemainderInstances)
  {
    const ezUInt8* pInputData = input.GetData<ezUInt8>();
    const ezUInt32 uiByteStride = input.GetElementStride();

    if (uiByteStride == sizeof(ValueType) && std::is_same<ValueType, StreamType>::value)
    {
      while (r != pRe)
      {
        r->template Load<4>(reinterpret_cast<const ValueType*>(pInputData));

        ++r;
        pInputData += sizeof(ValueType) * 4;
      }
    }
    else
    {
      ValueType x[4] = {};
      while (r != pRe)
      {
        x[0] = ReadInputData<ValueType, StreamType>(pInputData, uiByteStride);
        x[1] = ReadInputData<ValueType, StreamType>(pInputData, uiByteStride);
        x[2] = ReadInputData<ValueType, StreamType>(pInputData, uiByteStride);
        x[3] = ReadInputData<ValueType, StreamType>(pInputData, uiByteStride);

        r->template Load<4>(x);

        ++r;
      }
    }

    if (uiNumRemainderInstances > 0)
    {
      ValueType x[3];
      x[0] = ReadInputData<ValueType, StreamType>(pInputData, uiByteStride);
      x[1] = uiNumRemainderInstances >= 2 ? ReadInputData<ValueType, StreamType>(pInputData, uiByteStride) : x[0];
      x[2] = uiNumRemainderInstances >= 3 ? ReadInputData<ValueType, StreamType>(pInputData, uiByteStride) : x[1];

      r->Set(x[0], x[1], x[2], x[2]);
    }
  }

  template <typename ValueType, typename StreamType>
  EZ_ALWAYS_INLINE void StoreOutputData(ezUInt8*& ref_pData, ezUInt32 uiStride, ValueType value)
  {
    *reinterpret_cast<StreamType*>(ref_pData) = static_cast<StreamType>(value);
    ref_pData += uiStride;
  }

  template <typename RegisterType, typename ValueType, typename StreamType>
  void StoreOutput(RegisterType* r, RegisterType* pRe, ezProcessingStream& ref_output, ezUInt32 uiNumRemainderInstances)
  {
    ezUInt8* pOutputData = ref_output.GetWritableData<ezUInt8>();
    const ezUInt32 uiByteStride = ref_output.GetElementStride();

    if (uiByteStride == sizeof(ValueType) && std::is_same<ValueType, StreamType>::value)
    {
      while (r != pRe)
      {
        r->template Store<4>(reinterpret_cast<ValueType*>(pOutputData));

        ++r;
        pOutputData += sizeof(ValueType) * 4;
      }
    }
    else
    {
      ValueType x[4] = {};
      while (r != pRe)
      {
        r->template Store<4>(x);

        StoreOutputData<ValueType, StreamType>(pOutputData, uiByteStride, x[0]);
        StoreOutputData<ValueType, StreamType>(pOutputData, uiByteStride, x[1]);
        StoreOutputData<ValueType, StreamType>(pOutputData, uiByteStride, x[2]);
        StoreOutputData<ValueType, StreamType>(pOutputData, uiByteStride, x[3]);

        ++r;
      }
    }

    if (uiNumRemainderInstances > 0)
    {
      ValueType x[4];
      r->template Store<4>(x);

      for (ezUInt32 i = 0; i < uiNumRemainderInstances; ++i)
      {
        StoreOutputData<ValueType, StreamType>(pOutputData, uiByteStride, x[i]);
      }
    }
  }

  void VM_LoadF_4(const ByteCodeType*& pByteCode, ExecutionContext& context)
  {
    const ezUInt32 uiNumRemainderInstances = context.m_uiNumInstances & 0x3;

    DEFINE_TARGET_REGISTER();
    if (uiNumRemainderInstances > 0)
      --re;

    const ezUInt32 uiInputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode);
    auto& input = *context.m_Inputs[uiInputIndex];

    if (input.GetDataType() == ezProcessingStream::DataType::Float)
    {
      LoadInput<ezSimdVec4f, float, float>(reinterpret_cast<ezSimdVec4f*>(r), reinterpret_cast<ezSimdVec4f*>(re), input, uiNumRemainderInstances);
    }
    else
    {
      EZ_ASSERT_DEBUG(input.GetDataType() == ezProcessingStream::DataType::Half, "Unsupported input type '{}' for LoadF instruction", ezProcessingStream::GetDataTypeName(input.GetDataType()));
      LoadInput<ezSimdVec4f, float, ezFloat16>(reinterpret_cast<ezSimdVec4f*>(r), reinterpret_cast<ezSimdVec4f*>(re), input, uiNumRemainderInstances);
    }
  }

  void VM_LoadI_4(const ByteCodeType*& pByteCode, ExecutionContext& context)
  {
    const ezUInt32 uiNumRemainderInstances = context.m_uiNumInstances & 0x3;

    DEFINE_TARGET_REGISTER();
    if (uiNumRemainderInstances > 0)
      --re;

    const ezUInt32 uiInputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode);
    auto& input = *context.m_Inputs[uiInputIndex];

    if (input.GetDataType() == ezProcessingStream::DataType::Int)
    {
      LoadInput<ezSimdVec4i, int, int>(reinterpret_cast<ezSimdVec4i*>(r), reinterpret_cast<ezSimdVec4i*>(re), input, uiNumRemainderInstances);
    }
    else if (input.GetDataType() == ezProcessingStream::DataType::Short)
    {
      LoadInput<ezSimdVec4i, int, ezInt16>(reinterpret_cast<ezSimdVec4i*>(r), reinterpret_cast<ezSimdVec4i*>(re), input, uiNumRemainderInstances);
    }
    else
    {
      EZ_ASSERT_DEBUG(input.GetDataType() == ezProcessingStream::DataType::Byte, "Unsupported input type '{}' for LoadI instruction", ezProcessingStream::GetDataTypeName(input.GetDataType()));
      LoadInput<ezSimdVec4i, int, ezInt8>(reinterpret_cast<ezSimdVec4i*>(r), reinterpret_cast<ezSimdVec4i*>(re), input, uiNumRemainderInstances);
    }
  }

  void VM_StoreF_4(const ByteCodeType*& pByteCode, ExecutionContext& context)
  {
    const ezUInt32 uiNumRemainderInstances = context.m_uiNumInstances & 0x3;

    ezUInt32 uiOutputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode);
    auto& output = *context.m_Outputs[uiOutputIndex];

    // actually not target register but operand register in the is case, but we need something to loop over so we use the target register macro here.
    DEFINE_TARGET_REGISTER();
    if (uiNumRemainderInstances > 0)
      --re;

    if (output.GetDataType() == ezProcessingStream::DataType::Float)
    {
      StoreOutput<ezSimdVec4f, float, float>(reinterpret_cast<ezSimdVec4f*>(r), reinterpret_cast<ezSimdVec4f*>(re), output, uiNumRemainderInstances);
    }
    else
    {
      EZ_ASSERT_DEBUG(output.GetDataType() == ezProcessingStream::DataType::Half, "Unsupported input type '{}' for StoreF instruction", ezProcessingStream::GetDataTypeName(output.GetDataType()));
      StoreOutput<ezSimdVec4f, float, ezFloat16>(reinterpret_cast<ezSimdVec4f*>(r), reinterpret_cast<ezSimdVec4f*>(re), output, uiNumRemainderInstances);
    }
  }

  void VM_StoreI_4(const ByteCodeType*& pByteCode, ExecutionContext& context)
  {
    const ezUInt32 uiNumRemainderInstances = context.m_uiNumInstances & 0x3;

    ezUInt32 uiOutputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode);
    auto& output = *context.m_Outputs[uiOutputIndex];

    // actually not target register but operand register in the is case, but we need something to loop over so we use the target register macro here.
    DEFINE_TARGET_REGISTER();
    if (uiNumRemainderInstances > 0)
      --re;

    if (output.GetDataType() == ezProcessingStream::DataType::Int)
    {
      StoreOutput<ezSimdVec4i, int, int>(reinterpret_cast<ezSimdVec4i*>(r), reinterpret_cast<ezSimdVec4i*>(re), output, uiNumRemainderInstances);
    }
    else if (output.GetDataType() == ezProcessingStream::DataType::Short)
    {
      StoreOutput<ezSimdVec4i, int, ezInt16>(reinterpret_cast<ezSimdVec4i*>(r), reinterpret_cast<ezSimdVec4i*>(re), output, uiNumRemainderInstances);
    }
    else
    {
      EZ_ASSERT_DEBUG(output.GetDataType() == ezProcessingStream::DataType::Byte, "Unsupported input type '{}' for StoreI instruction", ezProcessingStream::GetDataTypeName(output.GetDataType()));
      StoreOutput<ezSimdVec4i, int, ezInt8>(reinterpret_cast<ezSimdVec4i*>(r), reinterpret_cast<ezSimdVec4i*>(re), output, uiNumRemainderInstances);
    }
  }

  void VM_Call(const ByteCodeType*& pByteCode, ExecutionContext& context)
  {
    EZ_WARNING_PUSH()
    EZ_WARNING_DISABLE_MSVC(4189)

    ezUInt32 uiFunctionIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode);
    auto& function = *context.m_Functions[uiFunctionIndex];

    DEFINE_TARGET_REGISTER();
    ezUInt32 uiNumArgs = ezExpressionByteCode::GetFunctionArgCount(pByteCode);

    ezHybridArray<ezArrayPtr<const ezExpression::Register>, 32> inputs;
    inputs.Reserve(uiNumArgs);
    for (ezUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
    {
      DEFINE_OP_REGISTER(x);
      inputs.PushBack(ezMakeArrayPtr(x, context.m_uiNumSimd4Instances));
    }

    ezExpression::Output output = ezMakeArrayPtr(r, context.m_uiNumSimd4Instances);

    function.m_Func(inputs, output, *context.m_pGlobalData);

    EZ_WARNING_POP()
  }

  static constexpr OpFunc s_Simd4Funcs[] = {
    nullptr,         // Nop,

    nullptr,         // FirstUnary,

    &AbsF_4,         // AbsF_R,
    &AbsI_4,         // AbsI_R,
    &SqrtF_4,        // SqrtF_R,

    &ExpF_4,         // ExpF_R,
    &LnF_4,          // LnF_R,
    &Log2F_4,        // Log2F_R,
    &Log2I_4,        // Log2I_R,
    &Log10F_4,       // Log10F_R,
    &Pow2F_4,        // Pow2F_R,

    &SinF_4,         // SinF_R,
    &CosF_4,         // CosF_R,
    &TanF_4,         // TanF_R,

    &ASinF_4,        // ASinF_R,
    &ACosF_4,        // ACosF_R,
    &ATanF_4,        // ATanF_R,

    &RoundF_4,       // RoundF_R,
    &FloorF_4,       // FloorF_R,
    &CeilF_4,        // CeilF_R,
    &TruncF_4,       // TruncF_R,

    &NotI_4,         // NotI_R,
    &NotB_4,         // NotB_R,

    &IToF_4,         // IToF_R,
    &FToI_4,         // FToI_R,

    nullptr,         // LastUnary,
    nullptr,         // FirstBinary,

    &AddF_4<false>,  // AddF_RR,
    &AddI_4<false>,  // AddI_RR,

    &SubF_4<false>,  // SubF_RR,
    &SubI_4<false>,  // SubI_RR,

    &MulF_4<false>,  // MulF_RR,
    &MulI_4<false>,  // MulI_RR,

    &DivF_4<false>,  // DivF_RR,
    &DivI_4<false>,  // DivI_RR,

    &MinF_4<false>,  // MinF_RR,
    &MinI_4<false>,  // MinI_RR,

    &MaxF_4<false>,  // MaxF_RR,
    &MaxI_4<false>,  // MaxI_RR,

    &ShlI_4<false>,  // ShlI_RR,
    &ShrI_4<false>,  // ShrI_RR,
    &AndI_4<false>,  // AndI_RR,
    &XorI_4<false>,  // XorI_RR,
    &OrI_4<false>,   // OrI_RR,

    &EqF_4<false>,   // EqF_RR,
    &EqI_4<false>,   // EqI_RR,
    &EqB_4<false>,   // EqB_RR,

    &NEqF_4<false>,  // NEqF_RR,
    &NEqI_4<false>,  // NEqI_RR,
    &NEqB_4<false>,  // NEqB_RR,

    &LtF_4<false>,   // LtF_RR,
    &LtI_4<false>,   // LtI_RR,

    &LEqF_4<false>,  // LEqF_RR,
    &LEqI_4<false>,  // LEqI_RR,

    &GtF_4<false>,   // GtF_RR,
    &GtI_4<false>,   // GtI_RR,

    &GEqF_4<false>,  // GEqF_RR,
    &GEqI_4<false>,  // GEqI_RR,

    &AndB_4<false>,  // AndB_RR,
    &OrB_4<false>,   // OrB_RR,

    nullptr,         // LastBinary,
    nullptr,         // FirstBinaryWithConstant,

    &AddF_4<true>,   // AddF_RC,
    &AddI_4<true>,   // AddI_RC,

    &SubF_4<true>,   // SubF_RC,
    &SubI_4<true>,   // SubI_RC,

    &MulF_4<true>,   // MulF_RC,
    &MulI_4<true>,   // MulI_RC,

    &DivF_4<true>,   // DivF_RC,
    &DivI_4<true>,   // DivI_RC,

    &MinF_4<true>,   // MinF_RC,
    &MinI_4<true>,   // MinI_RC,

    &MaxF_4<true>,   // MaxF_RC,
    &MaxI_4<true>,   // MaxI_RC,

    &ShlI_C_4<true>, // ShlI_RC,
    &ShrI_C_4<true>, // ShrI_RC,
    &AndI_4<true>,   // AndI_RC,
    &XorI_4<true>,   // XorI_RC,
    &OrI_4<true>,    // OrI_RC,

    &EqF_4<true>,    // EqF_RC,
    &EqI_4<true>,    // EqI_RC,
    &EqB_4<true>,    // EqB_RC

    &NEqF_4<true>,   // NEqF_RC,
    &NEqI_4<true>,   // NEqI_RC,
    &NEqB_4<true>,   // NEqB_RC

    &LtF_4<true>,    // LtF_RC,
    &LtI_4<true>,    // LtI_RC

    &LEqF_4<true>,   // LEqF_RC,
    &LEqI_4<true>,   // LEqI_RC

    &GtF_4<true>,    // GtF_RC,
    &GtI_4<true>,    // GtI_RC

    &GEqF_4<true>,   // GEqF_RC,
    &GEqI_4<true>,   // GEqI_RC

    &AndB_4<true>,   // AndB_RC,
    &OrB_4<true>,    // OrB_RC,

    nullptr,         // LastBinaryWithConstant,
    nullptr,         // FirstTernary,

    &SelF_4,         // SelF_RRR,
    &SelI_4,         // SelI_RRR,
    &SelB_4,         // SelB_RRR,

    nullptr,         // LastTernary,
    nullptr,         // FirstSpecial,

    &VM_MovX_R_4,    // MovX_R,
    &VM_MovX_C_4,    // MovX_C,
    &VM_LoadF_4,     // LoadF,
    &VM_LoadI_4,     // LoadI,
    &VM_StoreF_4,    // StoreF,
    &VM_StoreI_4,    // StoreI,

    &VM_Call,        // Call,

    nullptr,         // LastSpecial,
  };

  static_assert(EZ_ARRAY_SIZE(s_Simd4Funcs) == ezExpressionByteCode::OpCode::Count);

} // namespace

#undef DEFINE_TARGET_REGISTER
#undef DEFINE_OP_REGISTER
#undef DEFINE_CONSTANT
#undef UNARY_OP_INNER_LOOP
#undef DEFINE_UNARY_OP
#undef BINARY_OP_INNER_LOOP
#undef DEFINE_BINARY_OP
#undef TERNARY_OP_INNER_LOOP
#undef DEFINE_TERNARY_OP
