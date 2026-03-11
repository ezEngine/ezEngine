#pragma once

/// \file

/// Gets the number of arguments of a variadic preprocessor macro.
/// If an empty __VA_ARGS__ is passed in, this will still return 1.
/// There is no perfect way to detect parameter lists with zero elements.
#ifndef EZ_VA_NUM_ARGS
#  define EZ_VA_NUM_ARGS(...) EZ_VA_NUM_ARGS_HELPER(__VA_ARGS__, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#  define EZ_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, N, ...) N
#endif


#define EZ_CALL_MACRO(macro, args) macro args


#define EZ_EXPAND_ARGS_1(op, a0) op(a0)
#define EZ_EXPAND_ARGS_2(op, a0, a1) op(a0) op(a1)
#define EZ_EXPAND_ARGS_3(op, a0, a1, a2) op(a0) op(a1) op(a2)
#define EZ_EXPAND_ARGS_4(op, a0, a1, a2, a3) op(a0) op(a1) op(a2) op(a3)
#define EZ_EXPAND_ARGS_5(op, a0, a1, a2, a3, a4) op(a0) op(a1) op(a2) op(a3) op(a4)
#define EZ_EXPAND_ARGS_6(op, a0, a1, a2, a3, a4, a5) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5)
#define EZ_EXPAND_ARGS_7(op, a0, a1, a2, a3, a4, a5, a6) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6)
#define EZ_EXPAND_ARGS_8(op, a0, a1, a2, a3, a4, a5, a6, a7) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7)
#define EZ_EXPAND_ARGS_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8)
#define EZ_EXPAND_ARGS_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9)
#define EZ_EXPAND_ARGS_11(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10)
#define EZ_EXPAND_ARGS_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11)

/// Variadic macro "dispatching" the arguments to the correct macro.
/// The number of arguments is found by using EZ_VA_NUM_ARGS(__VA_ARGS__)
#define EZ_EXPAND_ARGS(op, ...) EZ_CALL_MACRO(EZ_PP_CONCAT(EZ_EXPAND_ARGS_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define EZ_EXPAND_ARGS_COMMA_1(a0) a0
#define EZ_EXPAND_ARGS_COMMA_2(a0, a1) a0, a1
#define EZ_EXPAND_ARGS_COMMA_3(a0, a1, a2) a0, a1, a2
#define EZ_EXPAND_ARGS_COMMA_4(a0, a1, a2, a3) a0, a1, a2, a3
#define EZ_EXPAND_ARGS_COMMA_5(a0, a1, a2, a3, a4) a0, a1, a2, a3, a4
#define EZ_EXPAND_ARGS_COMMA_6(a0, a1, a2, a3, a4, a5) a0, a1, a2, a3, a4, a5
#define EZ_EXPAND_ARGS_COMMA_7(a0, a1, a2, a3, a4, a5, a6) a0 a1, a2, a3, a4, a5, a6
#define EZ_EXPAND_ARGS_COMMA_8(a0, a1, a2, a3, a4, a5, a6, a7) a0, a1, a2, a3, a4, a5, a6, a7
#define EZ_EXPAND_ARGS_COMMA_9(a0, a1, a2, a3, a4, a5, a6, a7, a8) a0, a1, a2, a3, a4, a5, a6, a7, a8
#define EZ_EXPAND_ARGS_COMMA_10(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) a0, a1, a2, a3, a4, a5, a6, a7, a8, a9
#define EZ_EXPAND_ARGS_COMMA_11(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10
#define EZ_EXPAND_ARGS_COMMA_12(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11

/// Variadic macro "dispatching" the arguments to the correct macro.
/// The number of arguments is found by using EZ_VA_NUM_ARGS(__VA_ARGS__)
#define EZ_EXPAND_ARGS_COMMA(...) EZ_CALL_MACRO(EZ_PP_CONCAT(EZ_EXPAND_ARGS_COMMA_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))

//////////////////////////////////////////////////////////////////////////
#define EZ_EXPAND_ARGS_WITH_INDEX_1(op, a0) op(a0, 0)
#define EZ_EXPAND_ARGS_WITH_INDEX_2(op, a0, a1) op(a0, 0) op(a1, 1)
#define EZ_EXPAND_ARGS_WITH_INDEX_3(op, a0, a1, a2) op(a0, 0) op(a1, 1) op(a2, 2)
#define EZ_EXPAND_ARGS_WITH_INDEX_4(op, a0, a1, a2, a3) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3)
#define EZ_EXPAND_ARGS_WITH_INDEX_5(op, a0, a1, a2, a3, a4) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4)
#define EZ_EXPAND_ARGS_WITH_INDEX_6(op, a0, a1, a2, a3, a4, a5) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5)
#define EZ_EXPAND_ARGS_WITH_INDEX_7(op, a0, a1, a2, a3, a4, a5, a6) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6)
#define EZ_EXPAND_ARGS_WITH_INDEX_8(op, a0, a1, a2, a3, a4, a5, a6, a7) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7)
#define EZ_EXPAND_ARGS_WITH_INDEX_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 8)
#define EZ_EXPAND_ARGS_WITH_INDEX_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 8) op(a9, 9)

#define EZ_EXPAND_ARGS_WITH_INDEX(op, ...) EZ_CALL_MACRO(EZ_PP_CONCAT(EZ_EXPAND_ARGS_WITH_INDEX_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define EZ_EXPAND_ARGS_PAIR_1(...)
#define EZ_EXPAND_ARGS_PAIR_2(op, a0, a1) op(a0, a1)
#define EZ_EXPAND_ARGS_PAIR_3(op, a0, a1, ...) op(a0, a1)
#define EZ_EXPAND_ARGS_PAIR_4(op, a0, a1, a2, a3) op(a0, a1) op(a2, a3)
#define EZ_EXPAND_ARGS_PAIR_6(op, a0, a1, a2, a3, a4, a5) op(a0, a1) op(a2, a3) op(a4, a5)
#define EZ_EXPAND_ARGS_PAIR_8(op, a0, a1, a2, a3, a4, a5, a6, a7) op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7)
#define EZ_EXPAND_ARGS_PAIR_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9)
#define EZ_EXPAND_ARGS_PAIR_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11)
#define EZ_EXPAND_ARGS_PAIR_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13)
#define EZ_EXPAND_ARGS_PAIR_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15)
#define EZ_EXPAND_ARGS_PAIR_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15) op(a16, a17)
#define EZ_EXPAND_ARGS_PAIR_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15) op(a16, a17) op(a18, a19)

#define EZ_EXPAND_ARGS_PAIR(op, ...) EZ_CALL_MACRO(EZ_PP_CONCAT(EZ_EXPAND_ARGS_PAIR_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define EZ_EXPAND_ARGS_PAIR_COMMA_1(...) /* handles the case of zero parameters (e.g. an empty __VA_ARGS__) */
#define EZ_EXPAND_ARGS_PAIR_COMMA_2(op, a0, a1) op(a0, a1)
#define EZ_EXPAND_ARGS_PAIR_COMMA_3(op, a0, a1, ...) op(a0, a1)
#define EZ_EXPAND_ARGS_PAIR_COMMA_4(op, a0, a1, a2, a3) op(a0, a1), op(a2, a3)
#define EZ_EXPAND_ARGS_PAIR_COMMA_6(op, a0, a1, a2, a3, a4, a5) op(a0, a1), op(a2, a3), op(a4, a5)
#define EZ_EXPAND_ARGS_PAIR_COMMA_8(op, a0, a1, a2, a3, a4, a5, a6, a7) op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7)
#define EZ_EXPAND_ARGS_PAIR_COMMA_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9)
#define EZ_EXPAND_ARGS_PAIR_COMMA_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11)
#define EZ_EXPAND_ARGS_PAIR_COMMA_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13)
#define EZ_EXPAND_ARGS_PAIR_COMMA_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15)
#define EZ_EXPAND_ARGS_PAIR_COMMA_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17)
#define EZ_EXPAND_ARGS_PAIR_COMMA_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17), op(a18, a19)
#define EZ_EXPAND_ARGS_PAIR_COMMA_22(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17), op(a18, a19), op(a20, a21)
#define EZ_EXPAND_ARGS_PAIR_COMMA_24(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17), op(a18, a19), op(a20, a21), op(a22, a23)
#define EZ_EXPAND_ARGS_PAIR_COMMA_26(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17), op(a18, a19), op(a20, a21), op(a22, a23), op(a24, a25)

#define EZ_EXPAND_ARGS_PAIR_COMMA(op, ...) EZ_CALL_MACRO(EZ_PP_CONCAT(EZ_EXPAND_ARGS_PAIR_COMMA_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define EZ_TO_BOOL_0 0
#define EZ_TO_BOOL_1 1
#define EZ_TO_BOOL_2 1
#define EZ_TO_BOOL_3 1
#define EZ_TO_BOOL_4 1
#define EZ_TO_BOOL_5 1
#define EZ_TO_BOOL_6 1
#define EZ_TO_BOOL_7 1
#define EZ_TO_BOOL_8 1
#define EZ_TO_BOOL_9 1

#define EZ_TO_BOOL(x) EZ_PP_CONCAT(EZ_TO_BOOL_, x)

//////////////////////////////////////////////////////////////////////////

#define EZ_IF_0(x)
#define EZ_IF_1(x) x
#define EZ_IF(cond, x)                   \
  EZ_PP_CONCAT(EZ_IF_, EZ_TO_BOOL(cond)) \
  (x)

#define EZ_IF_ELSE_0(x, y) y
#define EZ_IF_ELSE_1(x, y) x
#define EZ_IF_ELSE(cond, x, y)                \
  EZ_PP_CONCAT(EZ_IF_ELSE_, EZ_TO_BOOL(cond)) \
  (x, y)

//////////////////////////////////////////////////////////////////////////

#define EZ_COMMA_MARK_0
#define EZ_COMMA_MARK_1 ,
#define EZ_COMMA_IF(cond) EZ_PP_CONCAT(EZ_COMMA_MARK_, EZ_TO_BOOL(cond))

//////////////////////////////////////////////////////////////////////////

#define EZ_LIST_0(x)
#define EZ_LIST_1(x) EZ_PP_CONCAT(x, 0)
#define EZ_LIST_2(x) EZ_LIST_1(x), EZ_PP_CONCAT(x, 1)
#define EZ_LIST_3(x) EZ_LIST_2(x), EZ_PP_CONCAT(x, 2)
#define EZ_LIST_4(x) EZ_LIST_3(x), EZ_PP_CONCAT(x, 3)
#define EZ_LIST_5(x) EZ_LIST_4(x), EZ_PP_CONCAT(x, 4)
#define EZ_LIST_6(x) EZ_LIST_5(x), EZ_PP_CONCAT(x, 5)
#define EZ_LIST_7(x) EZ_LIST_6(x), EZ_PP_CONCAT(x, 6)
#define EZ_LIST_8(x) EZ_LIST_7(x), EZ_PP_CONCAT(x, 7)
#define EZ_LIST_9(x) EZ_LIST_8(x), EZ_PP_CONCAT(x, 8)
#define EZ_LIST_10(x) EZ_LIST_9(x), EZ_PP_CONCAT(x, 9)

#define EZ_LIST(x, count)       \
  EZ_PP_CONCAT(EZ_LIST_, count) \
  (x)

//////////////////////////////////////////////////////////////////////////

#define EZ_PAIR_LIST_0(x, y)
#define EZ_PAIR_LIST_1(x, y) \
  EZ_PP_CONCAT(x, 0)         \
  EZ_PP_CONCAT(y, 0)
#define EZ_PAIR_LIST_2(x, y) EZ_PAIR_LIST_1(x, y), EZ_PP_CONCAT(x, 1) EZ_PP_CONCAT(y, 1)
#define EZ_PAIR_LIST_3(x, y) EZ_PAIR_LIST_2(x, y), EZ_PP_CONCAT(x, 2) EZ_PP_CONCAT(y, 2)
#define EZ_PAIR_LIST_4(x, y) EZ_PAIR_LIST_3(x, y), EZ_PP_CONCAT(x, 3) EZ_PP_CONCAT(y, 3)
#define EZ_PAIR_LIST_5(x, y) EZ_PAIR_LIST_4(x, y), EZ_PP_CONCAT(x, 4) EZ_PP_CONCAT(y, 4)
#define EZ_PAIR_LIST_6(x, y) EZ_PAIR_LIST_5(x, y), EZ_PP_CONCAT(x, 5) EZ_PP_CONCAT(y, 5)
#define EZ_PAIR_LIST_7(x, y) EZ_PAIR_LIST_6(x, y), EZ_PP_CONCAT(x, 6) EZ_PP_CONCAT(y, 6)
#define EZ_PAIR_LIST_8(x, y) EZ_PAIR_LIST_7(x, y), EZ_PP_CONCAT(x, 7) EZ_PP_CONCAT(y, 7)
#define EZ_PAIR_LIST_9(x, y) EZ_PAIR_LIST_8(x, y), EZ_PP_CONCAT(x, 8) EZ_PP_CONCAT(y, 8)
#define EZ_PAIR_LIST_10(x, y) EZ_PAIR_LIST_9(x, y), EZ_PP_CONCAT(x, 9) EZ_PP_CONCAT(y, 9)

#define EZ_PAIR_LIST(x, y, count)    \
  EZ_PP_CONCAT(EZ_PAIR_LIST_, count) \
  (x, y)
