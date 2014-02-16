
#pragma once

/// \brief Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator<< (ezStreamWriterBase& Stream, const ezAllocatorBase::Stats& rhs);

/// \brief Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator>> (ezStreamReaderBase& Stream, ezAllocatorBase::Stats& rhs);

class ezTime;

/// \brief Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator<< (ezStreamWriterBase& Stream, ezTime Value);

/// \brief Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator>> (ezStreamReaderBase& Stream, ezTime& Value);

