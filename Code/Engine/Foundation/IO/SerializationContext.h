#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Containers/Map.h>

template<typename SELF>
class ezSerializationContext
{
public:

  static SELF* GetReaderContext(ezStreamReaderBase* pStream)
  {
    auto it = s_ActiveReaderContexts.Find(pStream);
    if (it.IsValid())
      return it.Value();

    return nullptr;
  }

  static SELF* GetWriterContext(ezStreamWriterBase* pStream)
  {
    auto it = s_ActiveWriterContexts.Find(pStream);
    if (it.IsValid())
      return it.Value();

    return nullptr;
  }

protected:
  
  ezSerializationContext()
  {
    m_pReaderStream = nullptr;
    m_pWriterStream = nullptr;
  }

  ~ezSerializationContext()
  {
    UnregisterReader();
    UnregisterWriter();
  }

  void RegisterStream(ezStreamReaderBase* pStream)
  {
    m_pReaderStream = pStream;
    s_ActiveReaderContexts[pStream] = (SELF*) this;
  }

  void UnregisterReader()
  {
    s_ActiveReaderContexts.Erase(m_pReaderStream);
    m_pReaderStream = nullptr;
  }

  void RegisterStream(ezStreamWriterBase* pStream)
  {
    m_pWriterStream = pStream;
    s_ActiveWriterContexts[pStream] = (SELF*) this;
  }

  void UnregisterWriter()
  {
    s_ActiveWriterContexts.Erase(m_pWriterStream);
    m_pWriterStream = nullptr;
  }

private:
  ezStreamReaderBase* m_pReaderStream;
  ezStreamWriterBase* m_pWriterStream;

  static ezMap<ezStreamReaderBase*, SELF*> s_ActiveReaderContexts;
  static ezMap<ezStreamWriterBase*, SELF*> s_ActiveWriterContexts;
};


template<typename SELF>
typename ezMap<ezStreamReaderBase*, SELF*> ezSerializationContext<SELF>::s_ActiveReaderContexts;

template<typename SELF>
typename ezMap<ezStreamWriterBase*, SELF*> ezSerializationContext<SELF>::s_ActiveWriterContexts;


#define EZ_ADD_SERIALIZATION_CONTEXT_OPERATORS(CONTEXT, TYPE) \
  \
ezStreamWriterBase& operator<<(ezStreamWriterBase& stream, const TYPE& type) \
{ \
  CONTEXT* pContext = CONTEXT::GetWriterContext(&stream); \
  EZ_ASSERT(pContext != nullptr, "No serialization context of type '" #CONTEXT "' that handles type '" #TYPE "' is available."); \
  \
  pContext->Write(stream, type); \
  \
  return stream; \
} \
\
ezStreamReaderBase& operator>>(ezStreamReaderBase& stream, TYPE& type) \
{ \
  CONTEXT* pContext = CONTEXT::GetReaderContext(&stream); \
  EZ_ASSERT(pContext != nullptr, "No serialization context of type '" #CONTEXT "' that handles type '" #TYPE "' is available."); \
\
  pContext->Read(stream, type); \
\
  return stream; \
} \




