#include <PCH.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/Archive.h>
#include <Foundation/IO/MemoryStream.h>

class TypeA : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(TypeA);

public:

  ezInt32 m_iDataA;
  ezReflectedClass* m_pReference;

  TypeA()
  {
    m_pReference = this;
  }

  virtual void Serialize(ezArchiveWriter& stream) const override
  {
    stream << m_iDataA;
    stream.WriteObjectReference(m_pReference);
  }

  virtual void Deserialize(ezArchiveReader& stream) override
  {
    EZ_ASSERT_DEV(stream.GetStoredTypeVersion<TypeA>() == 2, "Wrong version");

    m_pReference = nullptr;

    stream >> m_iDataA;
    stream.ReadObjectReference((void**) &m_pReference);

    // the reference to self is already available at this point (in release builds)
#if EZ_DISABLED(EZ_COMPILE_FOR_DEBUG)
    EZ_TEST_BOOL(m_pReference == this);
#else
    EZ_TEST_BOOL(m_pReference == nullptr);
#endif
  }

  virtual void OnDeserialized() override
  {
    EZ_TEST_BOOL(m_pReference == this);
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TypeA, ezReflectedClass, 2, ezRTTIDefaultAllocator<TypeA>);
EZ_END_DYNAMIC_REFLECTED_TYPE();




class TypeAB : public TypeA
{
  EZ_ADD_DYNAMIC_REFLECTION(TypeAB);

public:

  ezInt32 m_iDataAB;

  virtual void Serialize(ezArchiveWriter& stream) const override
  {
    TypeA::Serialize(stream);

    stream << m_iDataAB;
  }

  virtual void Deserialize(ezArchiveReader& stream) override
  {
    EZ_ASSERT_DEV(stream.GetStoredTypeVersion<TypeAB>() == 3, "Wrong version");

    TypeA::Deserialize(stream);

    stream >> m_iDataAB;
  }

  virtual void OnDeserialized() override
  {
    TypeA::OnDeserialized();
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TypeAB, TypeA, 3, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();



class TypeABD : public TypeAB
{
  EZ_ADD_DYNAMIC_REFLECTION(TypeABD);

public:

  ezInt32 m_iDataABD;
  ezReflectedClass* pA;

  TypeABD()
  {
    pA = nullptr;
  }

  ~TypeABD()
  {
    if (pA)
    {
      pA->GetDynamicRTTI()->GetAllocator()->Deallocate(pA);
    }
  }

  virtual void Serialize(ezArchiveWriter& stream) const override
  {
    TypeAB::Serialize(stream);

    stream << m_iDataABD;

    TypeA a;
    a.m_iDataA = 13;
    stream.WriteReflectedObject(&a);
  }

  virtual void Deserialize(ezArchiveReader& stream) override
  {
    EZ_ASSERT_DEV(stream.GetStoredTypeVersion<TypeABD>() == 5, "Wrong version");

    TypeAB::Deserialize(stream);

    stream >> m_iDataABD;

    pA = stream.ReadReflectedObject();
    EZ_ASSERT_DEV(pA != nullptr, "bla");
    EZ_ASSERT_DEV(pA->GetDynamicRTTI() == ezGetStaticRTTI<TypeA>(), "Wrong type");
  }

  virtual void OnDeserialized() override
  {
    TypeAB::OnDeserialized();
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TypeABD, TypeAB, 5, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();



class TypeC : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(TypeC);

public:

  ezInt32 m_iDataC;

  virtual void Serialize(ezArchiveWriter& stream) const override
  {
    stream << m_iDataC;

    TypeA a;
    a.m_iDataA = 17;
    stream.WriteReflectedObject(&a);
  }

  virtual void Deserialize(ezArchiveReader& stream) override
  {
    EZ_ASSERT_DEV(stream.GetStoredTypeVersion<TypeC>() == 4, "Wrong version");

    stream >> m_iDataC;

    ezReflectedClass* pA = stream.ReadReflectedObject();
    EZ_ASSERT_DEV(pA != nullptr, "bla");

    pA->GetDynamicRTTI()->GetAllocator()->Deallocate(pA);
  }

  virtual void OnDeserialized() override
  {
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TypeC, ezReflectedClass, 4, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


class SerializerAB : public ezArchiveSerializer
{
public:

  virtual void Serialize(ezArchiveWriter& stream, const ezRTTI* pRtti, const void* pReference) override
  {
    stream << pRtti->GetTypeName();
  }

  virtual void* Deserialize(ezArchiveReader& stream, const ezRTTI* pRtti) override
  {
    ezString s;
    stream >> s;

    EZ_ASSERT_DEV(s == pRtti->GetTypeName(), "Wrong type");

    if (s == "TypeAB")
      return new TypeAB;

    if (s == "TypeABD")
      return new TypeABD;

    EZ_REPORT_FAILURE("Unknown type");
    return nullptr;
  }

};


EZ_CREATE_SIMPLE_TEST(IO, Archive)
{
  ezMemoryStreamStorage StreamStorage;

  ezMemoryStreamWriter MemoryWriter(&StreamStorage);
  ezMemoryStreamReader MemoryReader(&StreamStorage);

  ezVec3 TempWrite(23, 42, 88);
  ezVec3* TempRead = &TempWrite;

  SerializerAB SerAB;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write Format")
  {
    ezArchiveWriter writer(MemoryWriter);

    writer.RegisterTypeSerializer(ezGetStaticRTTI<TypeAB>(), &SerAB);

    writer.BeginStream();

    {
      writer.BeginChunk("Chunk1", 1);

      writer.WriteObjectReference(&TempWrite);

      ezVec3 v1(1, 2, 3);
      writer.BeginTypedObject(ezGetStaticRTTI<ezVec3>(), &v1);
      writer << v1;
      writer.EndTypedObject();

      TypeA a;
      TypeC c;
      TypeAB ab;
      TypeABD abd;

      a.m_iDataA = 12;
      writer.WriteReflectedObject(&a); // write with type information

      writer.WriteObjectReference(&a);
      writer.WriteObjectReference(&c);
      writer.WriteObjectReference(&ab);
      writer.WriteObjectReference(&abd);
      
      c.m_iDataC = 31;
      writer.WriteReflectedObject(&c); // write with type information

      
      ab.m_iDataA = 13;
      ab.m_iDataAB = 21;
      writer.WriteReflectedObject(&ab); // write with type information

      
      abd.m_iDataA = 14;
      abd.m_iDataAB = 22;
      abd.m_iDataABD = 41;
      writer.WriteReflectedObject(&abd); // write with type information


      writer.EndChunk();
    }

    writer.EndStream();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read Format")
  {
    ezArchiveReader reader(MemoryReader);

    reader.RegisterTypeSerializer(ezGetStaticRTTI<TypeAB>(), &SerAB);

    reader.BeginStream();

    TypeA* pTypeA;
    TypeAB* pTypeAB;
    TypeABD* pTypeABD;

    TypeA* pRefTypeA;
    TypeC* pRefTypeC;
    TypeAB* pRefTypeAB;
    TypeABD* pRefTypeABD;

    {
      EZ_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      EZ_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk1");
      EZ_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 1);

      const ezUInt32 uiObjectID = reader.ReadObjectReference((void**) &TempRead);

      EZ_TEST_BOOL(TempRead == nullptr);

      reader.SetObjectReferenceMapping(uiObjectID, &TempWrite);

      void* pVec = reader.ReadTypedObject();
      EZ_TEST_BOOL(pVec == nullptr); // vec3 has no default allocator

      ezReflectedClass* pA = reader.ReadReflectedObject();
      EZ_TEST_BOOL(pA->GetDynamicRTTI() == ezGetStaticRTTI<TypeA>());
      pTypeA = (TypeA*) pA;
      EZ_TEST_INT(pTypeA->m_iDataA, 12);
      
      
      reader.ReadObjectReference((void**) &pRefTypeA);
      reader.ReadObjectReference((void**) &pRefTypeC);
      reader.ReadObjectReference((void**) &pRefTypeAB);
      reader.ReadObjectReference((void**) &pRefTypeABD);

      // pRefTypeA may already be set
      EZ_TEST_BOOL(pRefTypeC == nullptr);
      EZ_TEST_BOOL(pRefTypeAB == nullptr);
      EZ_TEST_BOOL(pRefTypeABD == nullptr);


      void* pC = reader.ReadReflectedObject();
      EZ_TEST_BOOL(pC == nullptr); // TypeC has no allocator

      ezReflectedClass* pAB = reader.ReadReflectedObject(); // uses the serializer
      EZ_TEST_BOOL(pAB->GetDynamicRTTI() == ezGetStaticRTTI<TypeAB>());
      pTypeAB = (TypeAB*) pAB;
      EZ_TEST_INT(pTypeAB->m_iDataA, 13);
      EZ_TEST_INT(pTypeAB->m_iDataAB, 21);


      ezReflectedClass* pABD = reader.ReadReflectedObject(); // also uses the serializer (derived class)
      EZ_TEST_BOOL(pABD->GetDynamicRTTI() == ezGetStaticRTTI<TypeABD>());
      pTypeABD = (TypeABD*) pABD;
      EZ_TEST_INT(pTypeABD->m_iDataA, 14);
      EZ_TEST_INT(pTypeABD->m_iDataAB, 22);
      EZ_TEST_INT(pTypeABD->m_iDataABD, 41);




      reader.NextChunk();
    }

    EZ_TEST_BOOL(!reader.GetCurrentChunk().m_bValid);

    EZ_TEST_BOOL(TempRead == nullptr);

    EZ_TEST_BOOL(pRefTypeC == nullptr);
    EZ_TEST_BOOL(pRefTypeAB == nullptr);
    EZ_TEST_BOOL(pRefTypeABD == nullptr);

    reader.EndStream();

    // after EndStream all references are resolved

    EZ_TEST_BOOL(TempRead == &TempWrite);

    EZ_TEST_BOOL(pRefTypeA == pTypeA);
    EZ_TEST_BOOL(pRefTypeC == nullptr);
    EZ_TEST_BOOL(pRefTypeAB == pTypeAB);
    EZ_TEST_BOOL(pRefTypeABD == pTypeABD);

    // now we can also call the OnDeserialized function
    reader.CallOnDeserialized();

    pTypeA->GetDynamicRTTI()->GetAllocator()->Deallocate(pTypeA);
    delete pTypeAB;
    delete pTypeABD;
  }

  int i = 0;
}





