struct ezJniModifiers
{
  enum Enum
  {
    PUBLIC = 1,
    PRIVATE = 2,
    PROTECTED = 4,
    STATIC = 8,
    FINAL = 16,
    SYNCHRONIZED = 32,
    VOLATILE = 64,
    TRANSIENT = 128,
    NATIVE = 256,
    INTERFACE = 512,
    ABSTRACT = 1024,
    STRICT = 2048,
  };
};

ezJniObject::ezJniObject(jobject object, ezJniOwnerShip ownerShip)
  : m_class(nullptr)
{
  switch (ownerShip)
  {
    case ezJniOwnerShip::OWN:
      m_object = object;
      m_own = true;
      break;

    case ezJniOwnerShip::COPY:
      m_object = ezJniAttachment::GetEnv()->NewLocalRef(object);
      m_own = true;
      break;

    case ezJniOwnerShip::BORROW:
      m_object = object;
      m_own = false;
      break;
  }
}

ezJniObject::ezJniObject(const ezJniObject& other)
  : m_class(nullptr)
{
  m_object = ezJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own = true;
}

ezJniObject::ezJniObject(ezJniObject&& other)
{
  m_object = other.m_object;
  m_class = other.m_class;
  m_own = other.m_own;

  other.m_object = nullptr;
  other.m_class = nullptr;
  other.m_own = false;
}

ezJniObject& ezJniObject::operator=(const ezJniObject& other)
{
  if (this == &other)
    return *this;

  Reset();
  m_object = ezJniAttachment::GetEnv()->NewLocalRef(other.m_object);
  m_own = true;
  return *this;
}

ezJniObject& ezJniObject::operator=(ezJniObject&& other)
{
  if (this == &other)
    return *this;

  Reset();

  m_object = other.m_object;
  m_class = other.m_class;
  m_own = other.m_own;

  other.m_object = nullptr;
  other.m_class = nullptr;
  other.m_own = false;

  return *this;
}

ezJniObject::~ezJniObject()
{
  Reset();
}

void ezJniObject::Reset()
{
  if (m_object && m_own)
  {
    ezJniAttachment::GetEnv()->DeleteLocalRef(m_object);
    m_object = nullptr;
    m_own = false;
  }
  if (m_class)
  {
    ezJniAttachment::GetEnv()->DeleteLocalRef(m_class);
    m_class = nullptr;
  }
}

jobject ezJniObject::GetJObject() const
{
  return m_object;
}

bool ezJniObject::operator==(const ezJniObject& other) const
{
  return ezJniAttachment::GetEnv()->IsSameObject(m_object, other.m_object) == JNI_TRUE;
}

bool ezJniObject::operator!=(const ezJniObject& other) const
{
  return !operator==(other);
}

// Template specializations to dispatch to the correct JNI method for each C++ type.
template <typename T, bool unused = false>
struct ezJniTraits
{
  static_assert(unused, "The passed C++ type is not supported by the JNI wrapper. Arguments and returns types must be one of bool, signed char/jbyte, unsigned short/jchar, short/jshort, int/jint, long long/jlong, float/jfloat, double/jdouble, ezJniObject, ezJniString or ezJniClass.");

  // Places the argument inside a jvalue union.
  static jvalue ToValue(T);

  // Retrieves the Java class static type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static ezJniClass GetStaticType();

  // Retrieves the Java class dynamic type of the argument. For primitives, this is not the boxed type, but the primitive type.
  static ezJniClass GetRuntimeType(T);

  // Creates an invalid/null object to return in case of errors.
  static T GetEmptyObject();

  // Call an instance method with the return type.
  template <typename... Args>
  static T CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  // Call a static method with the return type.
  template <typename... Args>
  static T CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  // Sets/gets a field of the type.
  static void SetField(jobject self, jfieldID field, T);
  static T GetField(jobject self, jfieldID field);

  // Sets/gets a static field of the type.
  static void SetStaticField(jclass clazz, jfieldID field, T);
  static T GetStaticField(jclass clazz, jfieldID field);

  // Appends the JNI type signature of this type to the string buf
  static bool AppendSignature(const T& obj, ezStringBuilder& str);
  static const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<bool>
{
  static inline jvalue ToValue(bool value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(bool);

  static inline bool GetEmptyObject();

  template <typename... Args>
  static bool CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static bool CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, bool arg);
  static inline bool GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, bool arg);
  static inline bool GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(bool, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<jbyte>
{
  static inline jvalue ToValue(jbyte value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(jbyte);

  static inline jbyte GetEmptyObject();

  template <typename... Args>
  static jbyte CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jbyte CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jbyte arg);
  static inline jbyte GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jbyte arg);
  static inline jbyte GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jbyte, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<jchar>
{
  static inline jvalue ToValue(jchar value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(jchar);

  static inline jchar GetEmptyObject();

  template <typename... Args>
  static jchar CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jchar CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jchar arg);
  static inline jchar GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jchar arg);
  static inline jchar GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jchar, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<jshort>
{
  static inline jvalue ToValue(jshort value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(jshort);

  static inline jshort GetEmptyObject();

  template <typename... Args>
  static jshort CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jshort CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jshort arg);
  static inline jshort GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jshort arg);
  static inline jshort GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jshort, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<jint>
{
  static inline jvalue ToValue(jint value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(jint);

  static inline jint GetEmptyObject();

  template <typename... Args>
  static jint CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jint CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jint arg);
  static inline jint GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jint arg);
  static inline jint GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jint, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<jlong>
{
  static inline jvalue ToValue(jlong value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(jlong);

  static inline jlong GetEmptyObject();

  template <typename... Args>
  static jlong CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jlong CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jlong arg);
  static inline jlong GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jlong arg);
  static inline jlong GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jlong, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<jfloat>
{
  static inline jvalue ToValue(jfloat value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(jfloat);

  static inline jfloat GetEmptyObject();

  template <typename... Args>
  static jfloat CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jfloat CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jfloat arg);
  static inline jfloat GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jfloat arg);
  static inline jfloat GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jfloat, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<jdouble>
{
  static inline jvalue ToValue(jdouble value);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(jdouble);

  static inline jdouble GetEmptyObject();

  template <typename... Args>
  static jdouble CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static jdouble CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, jdouble arg);
  static inline jdouble GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, jdouble arg);
  static inline jdouble GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(jdouble, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<ezJniObject>
{
  static inline jvalue ToValue(const ezJniObject& object);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(const ezJniObject& object);

  static inline ezJniObject GetEmptyObject();

  template <typename... Args>
  static ezJniObject CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static ezJniObject CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const ezJniObject& arg);
  static inline ezJniObject GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const ezJniObject& arg);
  static inline ezJniObject GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const ezJniObject& obj, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<ezJniClass>
{
  static inline jvalue ToValue(const ezJniClass& object);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(const ezJniClass& object);

  static inline ezJniClass GetEmptyObject();

  template <typename... Args>
  static ezJniClass CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static ezJniClass CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const ezJniClass& arg);
  static inline ezJniClass GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const ezJniClass& arg);
  static inline ezJniClass GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const ezJniClass& obj, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<ezJniString>
{
  static inline jvalue ToValue(const ezJniString& object);

  static inline ezJniClass GetStaticType();

  static inline ezJniClass GetRuntimeType(const ezJniString& object);

  static inline ezJniString GetEmptyObject();

  template <typename... Args>
  static ezJniString CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static ezJniString CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline void SetField(jobject self, jfieldID field, const ezJniString& arg);
  static inline ezJniString GetField(jobject self, jfieldID field);

  static inline void SetStaticField(jclass clazz, jfieldID field, const ezJniString& arg);
  static inline ezJniString GetStaticField(jclass clazz, jfieldID field);

  static inline bool AppendSignature(const ezJniString& obj, ezStringBuilder& str);
  static inline const char* GetSignatureStatic();
};

template <>
struct ezJniTraits<void>
{
  static inline ezJniClass GetStaticType();

  static inline void GetEmptyObject();

  template <typename... Args>
  static void CallInstanceMethod(jobject self, jmethodID method, const Args&... args);

  template <typename... Args>
  static void CallStaticMethod(jclass clazz, jmethodID method, const Args&... args);

  static inline const char* GetSignatureStatic();
};

// Helpers to unpack variadic templates.
struct ezJniImpl
{
  static void CollectArgumentTypes(ezJniClass* target)
  {
  }

  template <typename T, typename... Tail>
  static void CollectArgumentTypes(ezJniClass* target, const T& arg, const Tail&... tail)
  {
    *target = ezJniTraits<T>::GetRuntimeType(arg);
    return ezJniImpl::CollectArgumentTypes(target + 1, tail...);
  }

  static void UnpackArgs(jvalue* target)
  {
  }

  template <typename T, typename... Tail>
  static void UnpackArgs(jvalue* target, const T& arg, const Tail&... tail)
  {
    *target = ezJniTraits<T>::ToValue(arg);
    return UnpackArgs(target + 1, tail...);
  }

  template <typename Ret, typename... Args>
  static bool BuildMethodSignature(ezStringBuilder& signature, const Args&... args)
  {
    signature.Append("(");
    if (!ezJniImpl::AppendSignature(signature, args...))
    {
      return false;
    }
    signature.Append(")");
    signature.Append(ezJniTraits<Ret>::GetSignatureStatic());
    return true;
  }

  static bool AppendSignature(ezStringBuilder& signature)
  {
    return true;
  }

  template <typename T, typename... Tail>
  static bool AppendSignature(ezStringBuilder& str, const T& arg, const Tail&... tail)
  {
    return ezJniTraits<T>::AppendSignature(arg, str) && AppendSignature(str, tail...);
  }
};

jvalue ezJniTraits<bool>::ToValue(bool value)
{
  jvalue result;
  result.z = value ? JNI_TRUE : JNI_FALSE;
  return result;
}

ezJniClass ezJniTraits<bool>::GetStaticType()
{
  return ezJniClass("java/lang/Boolean").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<bool>::GetRuntimeType(bool)
{
  return GetStaticType();
}

bool ezJniTraits<bool>::GetEmptyObject()
{
  return false;
}

template <typename... Args>
bool ezJniTraits<bool>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallBooleanMethodA(self, method, array) == JNI_TRUE;
}

template <typename... Args>
bool ezJniTraits<bool>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticBooleanMethodA(clazz, method, array) == JNI_TRUE;
}

void ezJniTraits<bool>::SetField(jobject self, jfieldID field, bool arg)
{
  return ezJniAttachment::GetEnv()->SetBooleanField(self, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool ezJniTraits<bool>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetBooleanField(self, field) == JNI_TRUE;
}

void ezJniTraits<bool>::SetStaticField(jclass clazz, jfieldID field, bool arg)
{
  return ezJniAttachment::GetEnv()->SetStaticBooleanField(clazz, field, arg ? JNI_TRUE : JNI_FALSE);
}

bool ezJniTraits<bool>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticBooleanField(clazz, field) == JNI_TRUE;
}

bool ezJniTraits<bool>::AppendSignature(bool, ezStringBuilder& str)
{
  str.Append("Z");
  return true;
}

const char* ezJniTraits<bool>::GetSignatureStatic()
{
  return "Z";
}

jvalue ezJniTraits<jbyte>::ToValue(jbyte value)
{
  jvalue result;
  result.b = value;
  return result;
}

ezJniClass ezJniTraits<jbyte>::GetStaticType()
{
  return ezJniClass("java/lang/Byte").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<jbyte>::GetRuntimeType(jbyte)
{
  return GetStaticType();
}

jbyte ezJniTraits<jbyte>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jbyte ezJniTraits<jbyte>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallByteMethodA(self, method, array);
}

template <typename... Args>
jbyte ezJniTraits<jbyte>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticByteMethodA(clazz, method, array);
}

void ezJniTraits<jbyte>::SetField(jobject self, jfieldID field, jbyte arg)
{
  return ezJniAttachment::GetEnv()->SetByteField(self, field, arg);
}

jbyte ezJniTraits<jbyte>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetByteField(self, field);
}

void ezJniTraits<jbyte>::SetStaticField(jclass clazz, jfieldID field, jbyte arg)
{
  return ezJniAttachment::GetEnv()->SetStaticByteField(clazz, field, arg);
}

jbyte ezJniTraits<jbyte>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticByteField(clazz, field);
}

bool ezJniTraits<jbyte>::AppendSignature(jbyte, ezStringBuilder& str)
{
  str.Append("B");
  return true;
}

const char* ezJniTraits<jbyte>::GetSignatureStatic()
{
  return "B";
}

jvalue ezJniTraits<jchar>::ToValue(jchar value)
{
  jvalue result;
  result.c = value;
  return result;
}

ezJniClass ezJniTraits<jchar>::GetStaticType()
{
  return ezJniClass("java/lang/Character").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<jchar>::GetRuntimeType(jchar)
{
  return GetStaticType();
}

jchar ezJniTraits<jchar>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jchar ezJniTraits<jchar>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallCharMethodA(self, method, array);
}

template <typename... Args>
jchar ezJniTraits<jchar>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticCharMethodA(clazz, method, array);
}

void ezJniTraits<jchar>::SetField(jobject self, jfieldID field, jchar arg)
{
  return ezJniAttachment::GetEnv()->SetCharField(self, field, arg);
}

jchar ezJniTraits<jchar>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetCharField(self, field);
}

void ezJniTraits<jchar>::SetStaticField(jclass clazz, jfieldID field, jchar arg)
{
  return ezJniAttachment::GetEnv()->SetStaticCharField(clazz, field, arg);
}

jchar ezJniTraits<jchar>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticCharField(clazz, field);
}

bool ezJniTraits<jchar>::AppendSignature(jchar, ezStringBuilder& str)
{
  str.Append("C");
  return true;
}

const char* ezJniTraits<jchar>::GetSignatureStatic()
{
  return "C";
}

jvalue ezJniTraits<jshort>::ToValue(jshort value)
{
  jvalue result;
  result.s = value;
  return result;
}

ezJniClass ezJniTraits<jshort>::GetStaticType()
{
  return ezJniClass("java/lang/Short").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<jshort>::GetRuntimeType(jshort)
{
  return GetStaticType();
}

jshort ezJniTraits<jshort>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jshort ezJniTraits<jshort>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallShortMethodA(self, method, array);
}

template <typename... Args>
jshort ezJniTraits<jshort>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticShortMethodA(clazz, method, array);
}

void ezJniTraits<jshort>::SetField(jobject self, jfieldID field, jshort arg)
{
  return ezJniAttachment::GetEnv()->SetShortField(self, field, arg);
}

jshort ezJniTraits<jshort>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetShortField(self, field);
}

void ezJniTraits<jshort>::SetStaticField(jclass clazz, jfieldID field, jshort arg)
{
  return ezJniAttachment::GetEnv()->SetStaticShortField(clazz, field, arg);
}

jshort ezJniTraits<jshort>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticShortField(clazz, field);
}

bool ezJniTraits<jshort>::AppendSignature(jshort, ezStringBuilder& str)
{
  str.Append("S");
  return true;
}

const char* ezJniTraits<jshort>::GetSignatureStatic()
{
  return "S";
}

jvalue ezJniTraits<jint>::ToValue(jint value)
{
  jvalue result;
  result.i = value;
  return result;
}

ezJniClass ezJniTraits<jint>::GetStaticType()
{
  return ezJniClass("java/lang/Integer").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<jint>::GetRuntimeType(jint)
{
  return GetStaticType();
}

jint ezJniTraits<jint>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jint ezJniTraits<jint>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallIntMethodA(self, method, array);
}

template <typename... Args>
jint ezJniTraits<jint>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticIntMethodA(clazz, method, array);
}

void ezJniTraits<jint>::SetField(jobject self, jfieldID field, jint arg)
{
  return ezJniAttachment::GetEnv()->SetIntField(self, field, arg);
}

jint ezJniTraits<jint>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetIntField(self, field);
}

void ezJniTraits<jint>::SetStaticField(jclass clazz, jfieldID field, jint arg)
{
  return ezJniAttachment::GetEnv()->SetStaticIntField(clazz, field, arg);
}

jint ezJniTraits<jint>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticIntField(clazz, field);
}

bool ezJniTraits<jint>::AppendSignature(jint, ezStringBuilder& str)
{
  str.Append("I");
  return true;
}

const char* ezJniTraits<jint>::GetSignatureStatic()
{
  return "I";
}

jvalue ezJniTraits<jlong>::ToValue(jlong value)
{
  jvalue result;
  result.j = value;
  return result;
}

ezJniClass ezJniTraits<jlong>::GetStaticType()
{
  return ezJniClass("java/lang/Long").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<jlong>::GetRuntimeType(jlong)
{
  return GetStaticType();
}

jlong ezJniTraits<jlong>::GetEmptyObject()
{
  return 0;
}

template <typename... Args>
jlong ezJniTraits<jlong>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallLongMethodA(self, method, array);
}

template <typename... Args>
jlong ezJniTraits<jlong>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticLongMethodA(clazz, method, array);
}

void ezJniTraits<jlong>::SetField(jobject self, jfieldID field, jlong arg)
{
  return ezJniAttachment::GetEnv()->SetLongField(self, field, arg);
}

jlong ezJniTraits<jlong>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetLongField(self, field);
}

void ezJniTraits<jlong>::SetStaticField(jclass clazz, jfieldID field, jlong arg)
{
  return ezJniAttachment::GetEnv()->SetStaticLongField(clazz, field, arg);
}

jlong ezJniTraits<jlong>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticLongField(clazz, field);
}

bool ezJniTraits<jlong>::AppendSignature(jlong, ezStringBuilder& str)
{
  str.Append("J");
  return true;
}

const char* ezJniTraits<jlong>::GetSignatureStatic()
{
  return "J";
}

jvalue ezJniTraits<jfloat>::ToValue(jfloat value)
{
  jvalue result;
  result.f = value;
  return result;
}

ezJniClass ezJniTraits<jfloat>::GetStaticType()
{
  return ezJniClass("java/lang/Float").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<jfloat>::GetRuntimeType(jfloat)
{
  return GetStaticType();
}

jfloat ezJniTraits<jfloat>::GetEmptyObject()
{
  return nanf("");
}

template <typename... Args>
jfloat ezJniTraits<jfloat>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallFloatMethodA(self, method, array);
}

template <typename... Args>
jfloat ezJniTraits<jfloat>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticFloatMethodA(clazz, method, array);
}

void ezJniTraits<jfloat>::SetField(jobject self, jfieldID field, jfloat arg)
{
  return ezJniAttachment::GetEnv()->SetFloatField(self, field, arg);
}

jfloat ezJniTraits<jfloat>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetFloatField(self, field);
}

void ezJniTraits<jfloat>::SetStaticField(jclass clazz, jfieldID field, jfloat arg)
{
  return ezJniAttachment::GetEnv()->SetStaticFloatField(clazz, field, arg);
}

jfloat ezJniTraits<jfloat>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticFloatField(clazz, field);
}

bool ezJniTraits<jfloat>::AppendSignature(jfloat, ezStringBuilder& str)
{
  str.Append("F");
  return true;
}

const char* ezJniTraits<jfloat>::GetSignatureStatic()
{
  return "F";
}

jvalue ezJniTraits<jdouble>::ToValue(jdouble value)
{
  jvalue result;
  result.d = value;
  return result;
}

ezJniClass ezJniTraits<jdouble>::GetStaticType()
{
  return ezJniClass("java/lang/Double").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

ezJniClass ezJniTraits<jdouble>::GetRuntimeType(jdouble)
{
  return GetStaticType();
}

jdouble ezJniTraits<jdouble>::GetEmptyObject()
{
  return nan("");
}

template <typename... Args>
jdouble ezJniTraits<jdouble>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallDoubleMethodA(self, method, array);
}

template <typename... Args>
jdouble ezJniTraits<jdouble>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticDoubleMethodA(clazz, method, array);
}

void ezJniTraits<jdouble>::SetField(jobject self, jfieldID field, jdouble arg)
{
  return ezJniAttachment::GetEnv()->SetDoubleField(self, field, arg);
}

jdouble ezJniTraits<jdouble>::GetField(jobject self, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetDoubleField(self, field);
}

void ezJniTraits<jdouble>::SetStaticField(jclass clazz, jfieldID field, jdouble arg)
{
  return ezJniAttachment::GetEnv()->SetStaticDoubleField(clazz, field, arg);
}

jdouble ezJniTraits<jdouble>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniAttachment::GetEnv()->GetStaticDoubleField(clazz, field);
}

bool ezJniTraits<jdouble>::AppendSignature(jdouble, ezStringBuilder& str)
{
  str.Append("D");
  return true;
}

const char* ezJniTraits<jdouble>::GetSignatureStatic()
{
  return "D";
}

jvalue ezJniTraits<ezJniObject>::ToValue(const ezJniObject& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

ezJniClass ezJniTraits<ezJniObject>::GetStaticType()
{
  return ezJniClass("java/lang/Object");
}

ezJniClass ezJniTraits<ezJniObject>::GetRuntimeType(const ezJniObject& arg)
{
  return arg.GetClass();
}

ezJniObject ezJniTraits<ezJniObject>::GetEmptyObject()
{
  return ezJniObject();
}

template <typename... Args>
ezJniObject ezJniTraits<ezJniObject>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniObject(ezJniAttachment::GetEnv()->CallObjectMethodA(self, method, array), ezJniOwnerShip::OWN);
}

template <typename... Args>
ezJniObject ezJniTraits<ezJniObject>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniObject(ezJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array), ezJniOwnerShip::OWN);
}

void ezJniTraits<ezJniObject>::SetField(jobject self, jfieldID field, const ezJniObject& arg)
{
  return ezJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

ezJniObject ezJniTraits<ezJniObject>::GetField(jobject self, jfieldID field)
{
  return ezJniObject(ezJniAttachment::GetEnv()->GetObjectField(self, field), ezJniOwnerShip::OWN);
}

void ezJniTraits<ezJniObject>::SetStaticField(jclass clazz, jfieldID field, const ezJniObject& arg)
{
  return ezJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

ezJniObject ezJniTraits<ezJniObject>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniObject(ezJniAttachment::GetEnv()->GetStaticObjectField(clazz, field), ezJniOwnerShip::OWN);
}

bool ezJniTraits<ezJniObject>::AppendSignature(const ezJniObject& obj, ezStringBuilder& str)
{
  if (obj.IsNull())
  {
    // Ensure null objects never generate valid signatures in order to force using the reflection path
    return false;
  }
  else
  {
    str.Append("L");
    str.Append(obj.GetClass().UnsafeCall<ezJniString>("getName", "()Ljava/lang/String;").GetData());
    str.ReplaceAll(".", "/");
    str.Append(";");
    return true;
  }
}

const char* ezJniTraits<ezJniObject>::GetSignatureStatic()
{
  return "Ljava/lang/Object;";
}

jvalue ezJniTraits<ezJniClass>::ToValue(const ezJniClass& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

ezJniClass ezJniTraits<ezJniClass>::GetStaticType()
{
  return ezJniClass("java/lang/Class");
}

ezJniClass ezJniTraits<ezJniClass>::GetRuntimeType(const ezJniClass& arg)
{
  // Assume there are no types derived from Class
  return GetStaticType();
}

ezJniClass ezJniTraits<ezJniClass>::GetEmptyObject()
{
  return ezJniClass();
}

template <typename... Args>
ezJniClass ezJniTraits<ezJniClass>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniClass(jclass(ezJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), ezJniOwnerShip::OWN);
}

template <typename... Args>
ezJniClass ezJniTraits<ezJniClass>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniClass(jclass(ezJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), ezJniOwnerShip::OWN);
}

void ezJniTraits<ezJniClass>::SetField(jobject self, jfieldID field, const ezJniClass& arg)
{
  return ezJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

ezJniClass ezJniTraits<ezJniClass>::GetField(jobject self, jfieldID field)
{
  return ezJniClass(jclass(ezJniAttachment::GetEnv()->GetObjectField(self, field)), ezJniOwnerShip::OWN);
}

void ezJniTraits<ezJniClass>::SetStaticField(jclass clazz, jfieldID field, const ezJniClass& arg)
{
  return ezJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

ezJniClass ezJniTraits<ezJniClass>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniClass(jclass(ezJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), ezJniOwnerShip::OWN);
}

bool ezJniTraits<ezJniClass>::AppendSignature(const ezJniClass& obj, ezStringBuilder& str)
{
  str.Append("Ljava/lang/Class;");
  return true;
}

const char* ezJniTraits<ezJniClass>::GetSignatureStatic()
{
  return "Ljava/lang/Class;";
}

jvalue ezJniTraits<ezJniString>::ToValue(const ezJniString& value)
{
  jvalue result;
  result.l = value.GetHandle();
  return result;
}

ezJniClass ezJniTraits<ezJniString>::GetStaticType()
{
  return ezJniClass("java/lang/String");
}

ezJniClass ezJniTraits<ezJniString>::GetRuntimeType(const ezJniString& arg)
{
  // Assume there are no types derived from String
  return GetStaticType();
}

ezJniString ezJniTraits<ezJniString>::GetEmptyObject()
{
  return ezJniString();
}

template <typename... Args>
ezJniString ezJniTraits<ezJniString>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniString(jstring(ezJniAttachment::GetEnv()->CallObjectMethodA(self, method, array)), ezJniOwnerShip::OWN);
}

template <typename... Args>
ezJniString ezJniTraits<ezJniString>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniString(jstring(ezJniAttachment::GetEnv()->CallStaticObjectMethodA(clazz, method, array)), ezJniOwnerShip::OWN);
}

void ezJniTraits<ezJniString>::SetField(jobject self, jfieldID field, const ezJniString& arg)
{
  return ezJniAttachment::GetEnv()->SetObjectField(self, field, arg.GetHandle());
}

ezJniString ezJniTraits<ezJniString>::GetField(jobject self, jfieldID field)
{
  return ezJniString(jstring(ezJniAttachment::GetEnv()->GetObjectField(self, field)), ezJniOwnerShip::OWN);
}

void ezJniTraits<ezJniString>::SetStaticField(jclass clazz, jfieldID field, const ezJniString& arg)
{
  return ezJniAttachment::GetEnv()->SetStaticObjectField(clazz, field, arg.GetHandle());
}

ezJniString ezJniTraits<ezJniString>::GetStaticField(jclass clazz, jfieldID field)
{
  return ezJniString(jstring(ezJniAttachment::GetEnv()->GetStaticObjectField(clazz, field)), ezJniOwnerShip::OWN);
}

bool ezJniTraits<ezJniString>::AppendSignature(const ezJniString& obj, ezStringBuilder& str)
{
  str.Append("Ljava/lang/String;");
  return true;
}

const char* ezJniTraits<ezJniString>::GetSignatureStatic()
{
  return "Ljava/lang/String;";
}

ezJniClass ezJniTraits<void>::GetStaticType()
{
  return ezJniClass("java/lang/Void").UnsafeGetStaticField<ezJniClass>("TYPE", "Ljava/lang/Class;");
}

void ezJniTraits<void>::GetEmptyObject()
{
  return;
}

template <typename... Args>
void ezJniTraits<void>::CallInstanceMethod(jobject self, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallVoidMethodA(self, method, array);
}

template <typename... Args>
void ezJniTraits<void>::CallStaticMethod(jclass clazz, jmethodID method, const Args&... args)
{
  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniAttachment::GetEnv()->CallStaticVoidMethodA(clazz, method, array);
}

const char* ezJniTraits<void>::GetSignatureStatic()
{
  return "V";
}

template <typename... Args>
ezJniObject ezJniClass::CreateInstance(const Args&... args) const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return ezJniObject();
  }

  const size_t N = sizeof...(args);

  ezJniClass inputTypes[N];
  ezJniImpl::CollectArgumentTypes(inputTypes, args...);

  ezJniObject foundMethod = FindConstructor(*this, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return ezJniObject();
  }

  jmethodID method = ezJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());

  jvalue array[sizeof...(args)];
  ezJniImpl::UnpackArgs(array, args...);
  return ezJniObject(ezJniAttachment::GetEnv()->NewObjectA(GetHandle(), method, array), ezJniOwnerShip::OWN);
}

template <typename Ret, typename... Args>
Ret ezJniClass::CallStatic(const char* name, const Args&... args) const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    ezLog::Error("Attempting to call static method '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  ezStringBuilder signature;
  if (ezJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = ezJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature.GetData());

    if (method)
    {
      return ezJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
    }
    else
    {
      ezJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  const size_t N = sizeof...(args);

  ezJniClass returnType = ezJniTraits<Ret>::GetStaticType();

  ezJniClass inputTypes[N];
  ezJniImpl::CollectArgumentTypes(inputTypes, args...);

  ezJniObject foundMethod = FindMethod(true, name, *this, returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = ezJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.GetHandle());
  return ezJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
}

template <typename Ret, typename... Args>
Ret ezJniClass::UnsafeCallStatic(const char* name, const char* signature, const Args&... args) const
{
  if (!GetJObject())
  {
    ezLog::Error("Attempting to call static method '{}' on null class.", name);
    ezLog::Error("Attempting to call static method '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = ezJniAttachment::GetEnv()->GetStaticMethodID(GetHandle(), name, signature);
  if (!method)
  {
    ezLog::Error("No such static method: '{}' with signature '{}' in class '{}'.", name, signature, ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_METHOD);
    return ezJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return ezJniTraits<Ret>::CallStaticMethod(GetHandle(), method, args...);
  }
}

template <typename Ret>
Ret ezJniClass::GetStaticField(const char* name) const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  if (!GetJObject())
  {
    ezLog::Error("Attempting to get static field '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = ezJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, ezJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return ezJniTraits<Ret>::GetStaticField(GetHandle(), fieldID);
  }
  else
  {
    ezJniAttachment::GetEnv()->ExceptionClear();
  }

  ezJniObject field = UnsafeCall<ezJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", ezJniString(name));

  if (ezJniAttachment::GetEnv()->ExceptionOccurred())
  {
    ezJniAttachment::GetEnv()->ExceptionClear();

    ezLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);

    return ezJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & ezJniModifiers::STATIC) == 0)
  {
    ezLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  ezJniClass fieldType = field.UnsafeCall<ezJniClass>("getType", "()Ljava/lang/Class;");

  ezJniClass returnType = ezJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    ezLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), ToString().GetData(), returnType.ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  return ezJniTraits<Ret>::GetStaticField(GetHandle(), ezJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret ezJniClass::UnsafeGetStaticField(const char* name, const char* signature) const
{
  if (!GetJObject())
  {
    ezLog::Error("Attempting to get static field '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID field = ezJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    ezLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return ezJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return ezJniTraits<Ret>::GetStaticField(GetHandle(), field);
  }
}

template <typename T>
void ezJniClass::SetStaticField(const char* name, const T& arg) const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!GetJObject())
  {
    ezLog::Error("Attempting to set static field '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  ezJniObject field = UnsafeCall<ezJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", ezJniString(name));

  if (ezJniAttachment::GetEnv()->ExceptionOccurred())
  {
    ezJniAttachment::GetEnv()->ExceptionClear();

    ezLog::Error("No field named '{}' found in class '{}'.", name, ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  ezJniClass modifierClass("java/lang/reflect/Modifier");
  jint modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & ezJniModifiers::STATIC) == 0)
  {
    ezLog::Error("Field named '{}' in class '{}' isn't static.", name, ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & ezJniModifiers::FINAL) != 0)
  {
    ezLog::Error("Field named '{}' in class '{}' is final.", name, ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  ezJniClass fieldType = field.UnsafeCall<ezJniClass>("getType", "()Ljava/lang/Class;");

  ezJniClass argType = ezJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      ezLog::Error("Field '{}' of type '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData());
      ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      ezLog::Error("Field '{}' of type '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), argType.ToString().GetData());
      ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return ezJniTraits<T>::SetStaticField(GetHandle(), ezJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void ezJniClass::UnsafeSetStaticField(const char* name, const char* signature, const T& arg) const
{
  if (!GetJObject())
  {
    ezLog::Error("Attempting to set static field '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = ezJniAttachment::GetEnv()->GetStaticFieldID(GetHandle(), name, signature);
  if (!field)
  {
    ezLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return ezJniTraits<T>::SetStaticField(GetHandle(), field, arg);
  }
}

template <typename Ret, typename... Args>
Ret ezJniObject::Call(const char* name, const Args&... args) const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    ezLog::Error("Attempting to call method '{}' on null object.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  // Fast path: Lookup method via signature built from parameters.
  // This only works for exact matches, but is roughly 50 times faster.
  ezStringBuilder signature;
  if (ezJniImpl::BuildMethodSignature<Ret>(signature, args...))
  {
    jmethodID method = ezJniAttachment::GetEnv()->GetMethodID(reinterpret_cast<jclass>(GetClass().GetHandle()), name, signature.GetData());

    if (method)
    {
      return ezJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
    }
    else
    {
      ezJniAttachment::GetEnv()->ExceptionClear();
    }
  }

  // Fallback to slow path using reflection
  const size_t N = sizeof...(args);

  ezJniClass returnType = ezJniTraits<Ret>::GetStaticType();

  ezJniClass inputTypes[N];
  ezJniImpl::CollectArgumentTypes(inputTypes, args...);

  ezJniObject foundMethod = FindMethod(false, name, GetClass(), returnType, inputTypes, N);

  if (foundMethod.IsNull())
  {
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = ezJniAttachment::GetEnv()->FromReflectedMethod(foundMethod.m_object);
  return ezJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
}

template <typename Ret, typename... Args>
Ret ezJniObject::UnsafeCall(const char* name, const char* signature, const Args&... args) const
{
  if (!m_object)
  {
    ezLog::Error("Attempting to call method '{}' on null object.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  jmethodID method = ezJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), name, signature);
  if (!method)
  {
    ezLog::Error("No such method: '{}' with signature '{}' in class '{}'.", name, signature, GetClass().ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_METHOD);
    return ezJniTraits<Ret>::GetEmptyObject();
  }
  else
  {
    return ezJniTraits<Ret>::CallInstanceMethod(m_object, method, args...);
  }
}

template <typename T>
void ezJniObject::SetField(const char* name, const T& arg) const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return;
  }

  if (!m_object)
  {
    ezLog::Error("Attempting to set field '{}' on null object.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  // No fast path here since we need to be able to report failures when attempting
  // to set final fields, which we can only do using reflection.

  ezJniObject field = GetClass().UnsafeCall<ezJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", ezJniString(name));

  if (ezJniAttachment::GetEnv()->ExceptionOccurred())
  {
    ezJniAttachment::GetEnv()->ExceptionClear();

    ezLog::Error("No field named '{}' found.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);

    return;
  }

  ezJniClass modifierClass("java/lang/reflect/Modifier");
  jint modifiers = field.UnsafeCall<jint>("getModifiers", "()I");

  if ((modifiers & ezJniModifiers::STATIC) != 0)
  {
    ezLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  if ((modifiers & ezJniModifiers::FINAL) != 0)
  {
    ezLog::Error("Field named '{}' in class '{}' is final.", name, GetClass().ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return;
  }

  ezJniClass fieldType = field.UnsafeCall<ezJniClass>("getType", "()Ljava/lang/Class;");

  ezJniClass argType = ezJniTraits<T>::GetRuntimeType(arg);

  if (argType.IsNull())
  {
    if (fieldType.IsPrimitive())
    {
      ezLog::Error("Field '{}' of type '{}'  in class '{}' can't be assigned null because it is a primitive type.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData());
      ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }
  else
  {
    if (!fieldType.IsAssignableFrom(argType))
    {
      ezLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned from type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), argType.ToString().GetData());
      ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
      return;
    }
  }

  return ezJniTraits<T>::SetField(m_object, ezJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()), arg);
}

template <typename T>
void ezJniObject::UnsafeSetField(const char* name, const char* signature, const T& arg) const
{
  if (!m_object)
  {
    ezLog::Error("Attempting to set field '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = ezJniAttachment::GetEnv()->GetFieldID(jclass(GetClass().GetHandle()), name, signature);
  if (!field)
  {
    ezLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return ezJniTraits<T>::SetField(m_object, field, arg);
  }
}

template <typename Ret>
Ret ezJniObject::GetField(const char* name) const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  if (!m_object)
  {
    ezLog::Error("Attempting to get field '{}' on null object.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  jfieldID fieldID = ezJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, ezJniTraits<Ret>::GetSignatureStatic());
  if (fieldID)
  {
    return ezJniTraits<Ret>::GetField(m_object, fieldID);
  }
  else
  {
    ezJniAttachment::GetEnv()->ExceptionClear();
  }

  ezJniObject field = GetClass().UnsafeCall<ezJniObject>("getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", ezJniString(name));

  if (ezJniAttachment::GetEnv()->ExceptionOccurred())
  {
    ezJniAttachment::GetEnv()->ExceptionClear();

    ezLog::Error("No field named '{}' found in class '{}'.", name, GetClass().ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);

    return ezJniTraits<Ret>::GetEmptyObject();
  }

  if ((field.UnsafeCall<jint>("getModifiers", "()I") & ezJniModifiers::STATIC) != 0)
  {
    ezLog::Error("Field named '{}' in class '{}' is static.", name, GetClass().ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  ezJniClass fieldType = field.UnsafeCall<ezJniClass>("getType", "()Ljava/lang/Class;");

  ezJniClass returnType = ezJniTraits<Ret>::GetStaticType();

  if (!returnType.IsAssignableFrom(fieldType))
  {
    ezLog::Error("Field '{}' of type '{}' in class '{}' can't be assigned to return type '{}'.", name, fieldType.ToString().GetData(), GetClass().ToString().GetData(), returnType.ToString().GetData());
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return ezJniTraits<Ret>::GetEmptyObject();
  }

  return ezJniTraits<Ret>::GetField(m_object, ezJniAttachment::GetEnv()->FromReflectedField(field.GetHandle()));
}

template <typename Ret>
Ret ezJniObject::UnsafeGetField(const char* name, const char* signature) const
{
  if (!m_object)
  {
    ezLog::Error("Attempting to get field '{}' on null class.", name);
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return;
  }

  jfieldID field = ezJniAttachment::GetEnv()->GetFieldID(GetClass().GetHandle(), name, signature);
  if (!field)
  {
    ezLog::Error("No such field: '{}' with signature '{}'.", name, signature);
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_FIELD);
    return;
  }
  else
  {
    return ezJniTraits<Ret>::GetField(m_object, field);
  }
}

template <>
struct ezJniTraits<ezJniNullPtr>
{

  static inline bool AppendSignature(const ezJniNullPtr& object, ezStringBuilder& str)
  {
    str.Append("L");
    str.Append(object.GetTypeSignature().GetData());
    str.Append(";");
    return true;
  }

  static inline jvalue ToValue(const ezJniNullPtr& object)
  {
    jvalue j;
    j.l = nullptr;
    return j;
  }

  static inline ezJniClass GetStaticType()
  {
    return ezJniClass("java/lang/Object");
  }

  static inline ezJniClass GetRuntimeType(const ezJniNullPtr& arg)
  {
    return ezJniClass(arg.GetTypeSignature().GetData());
  }
};
