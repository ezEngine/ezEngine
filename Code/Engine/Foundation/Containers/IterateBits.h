#pragma once

#include <Foundation/Containers/Implementation/BitIterator.h>

/// Helper base class to iterate over the bit indices or bit values of an integer.
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnsIndex If set, returns the index of the bit. Otherwise returns the value of the bit, i.e. EZ_BIT(value).
/// \tparam ReturnType Returned value type of the iterator.
/// \sa ezIterateBitValues, ezIterateBitIndices
template <typename DataType, bool ReturnsIndex, typename ReturnType = DataType>
struct ezIterateBits
{
  explicit ezIterateBits(DataType data)
  {
    m_Data = data;
  }

  ezBitIterator<DataType, ReturnsIndex, ReturnType> begin() const
  {
    return ezBitIterator<DataType, ReturnsIndex, ReturnType>(m_Data);
  };

  ezBitIterator<DataType, ReturnsIndex, ReturnType> end() const
  {
    return ezBitIterator<DataType, ReturnsIndex, ReturnType>();
  };

  DataType m_Data = {};
};

/// \brief Helper class to iterate over the bit values of an integer.
/// The class can iterate over the bits of any unsigned integer type that is equal to or smaller than ezUInt64.
/// \code{.cpp}
///    ezUInt64 bits = 0b1101;
///    for (auto bit : ezIterateBitValues(bits))
///    {
///      ezLog::Info("{}", bit); // Outputs 1, 4, 8
///    }
/// \endcode
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
template <typename DataType, typename ReturnType = DataType>
struct ezIterateBitValues : public ezIterateBits<DataType, false, ReturnType>
{
  explicit ezIterateBitValues(DataType data)
    : ezIterateBits<DataType, false, ReturnType>(data)
  {
  }
};

/// \brief Helper class to iterate over the bit indices of an integer.
/// The class can iterate over the bits of any unsigned integer type that is equal to or smaller than ezUInt64.
/// \code{.cpp}
///    ezUInt64 bits = 0b1101;
///    for (auto bit : ezIterateBitIndices(bits))
///    {
///      ezLog::Info("{}", bit); // Outputs 0, 2, 3
///    }
/// \endcode
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
template <typename DataType, typename ReturnType = DataType>
struct ezIterateBitIndices : public ezIterateBits<DataType, true, ReturnType>
{
  explicit ezIterateBitIndices(DataType data)
    : ezIterateBits<DataType, true, ReturnType>(data)
  {
  }
};
