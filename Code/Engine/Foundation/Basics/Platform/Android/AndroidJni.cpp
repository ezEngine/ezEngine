#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <Foundation/Threading/Thread.h>
#  include <android_native_app_glue.h>

thread_local JNIEnv* ezJniAttachment::s_env;
thread_local bool ezJniAttachment::s_ownsEnv;
thread_local int ezJniAttachment::s_attachCount;
thread_local ezJniErrorState ezJniAttachment::s_lastError;
thread_local ezJniErrorHandler s_onError;

ezJniAttachment::ezJniAttachment()
{
  if (s_attachCount > 0)
  {
    s_env->PushLocalFrame(16);
  }
  else
  {
    JNIEnv* env = nullptr;
    jint envStatus = ezAndroidUtils::GetAndroidJavaVM()->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    bool ownsEnv = (envStatus != JNI_OK);
    if (ownsEnv)
    {
      const char* szThreadName = "EZ JNI";
      if (const ezThread* pThread = ezThread::GetCurrentThread())
      {
        szThreadName = pThread->GetThreadName();
      }
      else if (ezThreadUtils::IsMainThread())
      {
        szThreadName = "EZ Main Thread";
      }
      // Assign name to attachment since ART complains about it not being set.
      JavaVMAttachArgs args = {JNI_VERSION_1_6, szThreadName, nullptr};
      ezAndroidUtils::GetAndroidJavaVM()->AttachCurrentThread(&env, &args);
    }
    else
    {
      // Assume already existing JNI environment will be alive as long as this object exists.
      EZ_ASSERT_DEV(env != nullptr, "");
      env->PushLocalFrame(16);
    }

    s_env = env;
    s_ownsEnv = ownsEnv;
  }

  s_attachCount++;
  EZ_ASSERT_ALWAYS(s_onError.IsValid() == false, "Can't install error handler for more than one instance.");
}

ezJniAttachment::~ezJniAttachment()
{
  s_onError = nullptr;
  s_attachCount--;

  if (s_attachCount == 0)
  {
    ClearLastError();

    if (s_ownsEnv)
    {
      ezAndroidUtils::GetAndroidJavaVM()->DetachCurrentThread();
    }
    else
    {
      s_env->PopLocalFrame(nullptr);
    }

    s_env = nullptr;
    s_ownsEnv = false;
  }
  else
  {
    s_env->PopLocalFrame(nullptr);
  }
}

ezJniObject ezJniAttachment::GetActivity()
{
  return ezJniObject(ezAndroidUtils::GetAndroidNativeActivity(), ezJniOwnerShip::BORROW);
}

JNIEnv* ezJniAttachment::GetEnv()
{
  EZ_ASSERT_DEV(s_env != nullptr, "Thread not attached to the JVM - you forgot to create an instance of ezJniAttachment in the current scope.");

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  void* unused;
  EZ_ASSERT_DEBUG(ezAndroidUtils::GetAndroidJavaVM()->GetEnv(&unused, JNI_VERSION_1_6) == JNI_OK,
    "Current thread has lost its attachment to the JVM - some OS calls can cause this to happen. Try to reduce the attachment to a smaller scope.");
#  endif

  return s_env;
}

ezJniErrorState ezJniAttachment::GetLastError()
{
  ezJniErrorState state = s_lastError;
  return state;
}

void ezJniAttachment::ClearLastError()
{
  s_lastError = ezJniErrorState::SUCCESS;
}

void ezJniAttachment::SetLastError(ezJniErrorState state)
{
  s_lastError = state;
  if (s_onError.IsValid() && s_lastError != ezJniErrorState::SUCCESS)
  {
    s_onError(s_lastError);
  }
}

bool ezJniAttachment::HasPendingException()
{
  return GetEnv()->ExceptionCheck();
}

void ezJniAttachment::ClearPendingException()
{
  return GetEnv()->ExceptionClear();
}

ezJniObject ezJniAttachment::GetPendingException()
{
  return ezJniObject(GetEnv()->ExceptionOccurred(), ezJniOwnerShip::OWN);
}

bool ezJniAttachment::FailOnPendingErrorOrException()
{
  if (ezJniAttachment::GetLastError() != ezJniErrorState::SUCCESS)
  {
    ezLog::Error("Aborting call because the previous error state was not cleared.");
    return true;
  }

  if (ezJniAttachment::HasPendingException())
  {
    ezLog::Error("Aborting call because a Java exception is still pending.");
    ezJniAttachment::SetLastError(ezJniErrorState::PENDING_EXCEPTION);
    return true;
  }

  return false;
}

void ezJniAttachment::InstallErrorHandler(ezJniErrorHandler onError)
{
  EZ_ASSERT_ALWAYS(s_attachCount == 1, "Can't install error handler for more than one instance.");
  s_onError = onError;
}

void ezJniObject::DumpTypes(const ezJniClass* inputTypes, int N, const ezJniClass* returnType)
{
  if (returnType != nullptr)
  {
    ezLog::Error("  With requested return type '{}'", returnType->ToString().GetData());
  }

  for (int paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    ezLog::Error("  With passed param type #{} '{}'", paramIdx, inputTypes[paramIdx].IsNull() ? "(null)" : inputTypes[paramIdx].ToString().GetData());
  }
}

int ezJniObject::CompareMethodSpecificity(const ezJniObject& method1, const ezJniObject& method2)
{
  ezJniClass returnType1 = method1.UnsafeCall<ezJniClass>("getReturnType", "()Ljava/lang/Class;");
  ezJniClass returnType2 = method2.UnsafeCall<ezJniClass>("getReturnType", "()Ljava/lang/Class;");

  ezJniObject paramTypes1 = method1.UnsafeCall<ezJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  ezJniObject paramTypes2 = method2.UnsafeCall<ezJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = ezJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = returnType1.IsAssignableFrom(returnType2) - returnType2.IsAssignableFrom(returnType1);

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    ezJniClass paramType1(
      jclass(ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), ezJniOwnerShip::OWN);
    ezJniClass paramType2(
      jclass(ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), ezJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool ezJniObject::IsMethodViable(bool bStatic, const ezJniObject& candidateMethod, const ezJniClass& returnType, ezJniClass* inputTypes, int N)
{
  // Check if staticness matches
  if (ezJniClass("java/lang/reflect/Modifier").UnsafeCallStatic<bool>("isStatic", "(I)Z", candidateMethod.UnsafeCall<int>("getModifiers", "()I")) !=
      bStatic)
  {
    return false;
  }

  // Check if return type is assignable to the requested type
  ezJniClass candidateReturnType = candidateMethod.UnsafeCall<ezJniClass>("getReturnType", "()Ljava/lang/Class;");
  if (!returnType.IsAssignableFrom(candidateReturnType))
  {
    return false;
  }

  // Check number of parameters
  ezJniObject parameterTypes = candidateMethod.UnsafeCall<ezJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize numCandidateParams = ezJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    ezJniClass paramType(
      jclass(ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), ezJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

ezJniObject ezJniObject::FindMethod(
  bool bStatic, const char* name, const ezJniClass& searchClass, const ezJniClass& returnType, ezJniClass* inputTypes, int N)
{
  if (searchClass.IsNull())
  {
    ezLog::Error("Attempting to find constructor for null type.");
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniObject();
  }

  ezHybridArray<ezJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    ezJniObject candidateMethod = searchClass.UnsafeCall<ezJniObject>(
      "getMethod", "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;", ezJniString(name), ezJniObject());

    if (!ezJniAttachment::GetEnv()->ExceptionCheck() && IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      ezJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    ezJniObject methodArray = searchClass.UnsafeCall<ezJniObject>("getMethods", "()[Ljava/lang/reflect/Method;");

    jsize numMethods = ezJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      ezJniObject candidateMethod(
        ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), ezJniOwnerShip::OWN);

      ezJniString methodName = candidateMethod.UnsafeCall<ezJniString>("getName", "()Ljava/lang/String;");

      if (strcmp(name, methodName.GetData()) != 0)
      {
        continue;
      }

      if (!IsMethodViable(bStatic, candidateMethod, returnType, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareMethodSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    ezLog::Error("Overload resolution failed: No method '{}' in class '{}' matches the requested return and parameter types.", name,
      searchClass.ToString().GetData());
    DumpTypes(inputTypes, N, &returnType);
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_METHOD);
    return ezJniObject();
  }
  else
  {
    ezLog::Error("Overload resolution failed: Call to '{}' in class '{}' is ambiguous. Cannot decide between the following candidates:", name,
      searchClass.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      ezLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, &returnType);
    ezJniAttachment::SetLastError(ezJniErrorState::AMBIGUOUS_CALL);
    return ezJniObject();
  }
}

int ezJniObject::CompareConstructorSpecificity(const ezJniObject& method1, const ezJniObject& method2)
{
  ezJniObject paramTypes1 = method1.UnsafeCall<ezJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  ezJniObject paramTypes2 = method2.UnsafeCall<ezJniObject>("getParameterTypes", "()[Ljava/lang/Class;");

  jsize N = ezJniAttachment::GetEnv()->GetArrayLength(jarray(paramTypes1.m_object));

  int decision = 0;

  for (jsize paramIdx = 0; paramIdx < N; ++paramIdx)
  {
    ezJniClass paramType1(
      jclass(ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes1.m_object), paramIdx)), ezJniOwnerShip::OWN);
    ezJniClass paramType2(
      jclass(ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(paramTypes2.m_object), paramIdx)), ezJniOwnerShip::OWN);

    int paramDecision = paramType1.IsAssignableFrom(paramType2) - paramType2.IsAssignableFrom(paramType1);

    if (decision == 0)
    {
      // No method is more specific yet
      decision = paramDecision;
    }
    else if (paramDecision != 0 && decision != paramDecision)
    {
      // There is no clear specificity ordering - one type is more specific, but the other less so
      return 0;
    }
  }

  return decision;
}

bool ezJniObject::IsConstructorViable(const ezJniObject& candidateMethod, ezJniClass* inputTypes, int N)
{
  // Check number of parameters
  ezJniObject parameterTypes = candidateMethod.UnsafeCall<ezJniObject>("getParameterTypes", "()[Ljava/lang/Class;");
  jsize numCandidateParams = ezJniAttachment::GetEnv()->GetArrayLength(jarray(parameterTypes.m_object));
  if (numCandidateParams != N)
  {
    return false;
  }

  // Check if input parameter types are assignable to the actual parameter types
  for (jsize paramIdx = 0; paramIdx < numCandidateParams; ++paramIdx)
  {
    ezJniClass paramType(
      jclass(ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(parameterTypes.m_object), paramIdx)), ezJniOwnerShip::OWN);

    if (inputTypes[paramIdx].IsNull())
    {
      if (paramType.IsPrimitive())
      {
        return false;
      }
    }
    else
    {
      if (!paramType.IsAssignableFrom(inputTypes[paramIdx]))
      {
        return false;
      }
    }
  }

  return true;
}

ezJniObject ezJniObject::FindConstructor(const ezJniClass& type, ezJniClass* inputTypes, int N)
{
  if (type.IsNull())
  {
    ezLog::Error("Attempting to find constructor for null type.");
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniObject();
  }

  ezHybridArray<ezJniObject, 32> bestCandidates;

  // In case of no parameters, fetch the method directly.
  if (N == 0)
  {
    ezJniObject candidateMethod =
      type.UnsafeCall<ezJniObject>("getConstructor", "([Ljava/lang/Class;)Ljava/lang/reflect/Constructor;", ezJniObject());

    if (!ezJniAttachment::GetEnv()->ExceptionCheck() && IsConstructorViable(candidateMethod, inputTypes, N))
    {
      bestCandidates.PushBack(candidateMethod);
    }
    else
    {
      ezJniAttachment::GetEnv()->ExceptionClear();
    }
  }
  else
  {
    // For methods with parameters, loop over all methods to find one with the correct name and matching parameter types

    ezJniObject methodArray = type.UnsafeCall<ezJniObject>("getConstructors", "()[Ljava/lang/reflect/Constructor;");

    jsize numMethods = ezJniAttachment::GetEnv()->GetArrayLength(jarray(methodArray.m_object));
    for (jsize methodIdx = 0; methodIdx < numMethods; ++methodIdx)
    {
      ezJniObject candidateMethod(
        ezJniAttachment::GetEnv()->GetObjectArrayElement(jobjectArray(methodArray.m_object), methodIdx), ezJniOwnerShip::OWN);

      if (!IsConstructorViable(candidateMethod, inputTypes, N))
      {
        continue;
      }

      bool isMoreSpecific = true;
      for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
      {
        int comparison = CompareConstructorSpecificity(bestCandidates[candidateIdx], candidateMethod);

        if (comparison == 1)
        {
          // Remove less specific candidate and continue looping
          bestCandidates.RemoveAtAndSwap(candidateIdx);
          candidateIdx--;
        }
        else if (comparison == -1)
        {
          // We're less specific, so by transitivity there are no other methods less specific than ours that we could throw out,
          // and we can abort the loop
          isMoreSpecific = false;
          break;
        }
        else
        {
          // No relation, so do nothing
        }
      }

      if (isMoreSpecific)
      {
        bestCandidates.PushBack(candidateMethod);
      }
    }
  }

  if (bestCandidates.GetCount() == 1)
  {
    return bestCandidates[0];
  }
  else if (bestCandidates.GetCount() == 0)
  {
    ezLog::Error("Overload resolution failed: No constructor in class '{}' matches the requested parameter types.", type.ToString().GetData());
    DumpTypes(inputTypes, N, nullptr);
    ezJniAttachment::SetLastError(ezJniErrorState::NO_MATCHING_METHOD);
    return ezJniObject();
  }
  else
  {
    ezLog::Error("Overload resolution failed: Call to constructor in class '{}' is ambiguous. Cannot decide between the following candidates:",
      type.ToString().GetData());
    for (int candidateIdx = 0; candidateIdx < bestCandidates.GetCount(); ++candidateIdx)
    {
      ezLog::Error("  Candidate #{}: '{}'", candidateIdx, bestCandidates[candidateIdx].ToString().GetData());
    }
    DumpTypes(inputTypes, N, nullptr);
    ezJniAttachment::SetLastError(ezJniErrorState::AMBIGUOUS_CALL);
    return ezJniObject();
  }
}

ezJniObject::ezJniObject()
  : m_object(nullptr)
  , m_class(nullptr)
  , m_own(false)
{
}

jobject ezJniObject::GetHandle() const
{
  return m_object;
}

ezJniClass ezJniObject::GetClass() const
{
  if (!m_object)
  {
    return ezJniClass();
  }

  if (!m_class)
  {
    const_cast<ezJniObject*>(this)->m_class = ezJniAttachment::GetEnv()->GetObjectClass(m_object);
  }

  return ezJniClass(m_class, ezJniOwnerShip::BORROW);
}

ezJniString ezJniObject::ToString() const
{
  if (ezJniAttachment::FailOnPendingErrorOrException())
  {
    return ezJniString();
  }

  // Implement ToString without UnsafeCall, since UnsafeCall requires ToString for diagnostic output.
  if (IsNull())
  {
    ezLog::Error("Attempting to call method 'toString' on null object.");
    ezJniAttachment::SetLastError(ezJniErrorState::CALL_ON_NULL_OBJECT);
    return ezJniString();
  }

  jmethodID method = ezJniAttachment::GetEnv()->GetMethodID(jclass(GetClass().m_object), "toString", "()Ljava/lang/String;");
  EZ_ASSERT_DEV(method, "Could not find JNI method toString()");

  return ezJniTraits<ezJniString>::CallInstanceMethod(m_object, method);
}

bool ezJniObject::IsInstanceOf(const ezJniClass& clazz) const
{
  if (IsNull())
  {
    return false;
  }

  return clazz.IsAssignableFrom(GetClass());
}

ezJniString::ezJniString()
  : ezJniObject()
  , m_utf(nullptr)
{
}

ezJniString::ezJniString(const char* str)
  : ezJniObject(ezJniAttachment::GetEnv()->NewStringUTF(str), ezJniOwnerShip::OWN)
  , m_utf(nullptr)
{
}

ezJniString::ezJniString(jstring string, ezJniOwnerShip ownerShip)
  : ezJniObject(string, ownerShip)
  , m_utf(nullptr)
{
}

ezJniString::ezJniString(const ezJniString& other)
  : ezJniObject(other)
  , m_utf(nullptr)
{
}

ezJniString::ezJniString(ezJniString&& other)
  : ezJniObject(other)
  , m_utf(nullptr)
{
  m_utf = other.m_utf;
  other.m_utf = nullptr;
}

ezJniString& ezJniString::operator=(const ezJniString& other)
{
  if (m_utf)
  {
    ezJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  ezJniObject::operator=(other);

  return *this;
}

ezJniString& ezJniString::operator=(ezJniString&& other)
{
  if (m_utf)
  {
    ezJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }

  ezJniObject::operator=(other);

  m_utf = other.m_utf;
  other.m_utf = nullptr;

  return *this;
}

ezJniString::~ezJniString()
{
  if (m_utf)
  {
    ezJniAttachment::GetEnv()->ReleaseStringUTFChars(jstring(GetJObject()), m_utf);
    m_utf = nullptr;
  }
}

const char* ezJniString::GetData() const
{
  if (IsNull())
  {
    ezLog::Error("Calling AsChar() on null Java String");
    return "<null>";
  }

  if (!m_utf)
  {
    const_cast<ezJniString*>(this)->m_utf = ezJniAttachment::GetEnv()->GetStringUTFChars(jstring(GetJObject()), nullptr);
  }

  return m_utf;
}


ezJniClass::ezJniClass()
  : ezJniObject()
{
}

ezJniClass::ezJniClass(const char* className)
  : ezJniObject(ezJniAttachment::GetEnv()->FindClass(className), ezJniOwnerShip::OWN)
{
  if (IsNull())
  {
    ezLog::Error("Class '{}' not found.", className);
    ezJniAttachment::SetLastError(ezJniErrorState::CLASS_NOT_FOUND);
  }
}

ezJniClass::ezJniClass(jclass clazz, ezJniOwnerShip ownerShip)
  : ezJniObject(clazz, ownerShip)
{
}

ezJniClass::ezJniClass(const ezJniClass& other)
  : ezJniObject(static_cast<const ezJniObject&>(other))
{
}

ezJniClass::ezJniClass(ezJniClass&& other)
  : ezJniObject(other)
{
}

ezJniClass& ezJniClass::operator=(const ezJniClass& other)
{
  ezJniObject::operator=(other);
  return *this;
}

ezJniClass& ezJniClass::operator=(ezJniClass&& other)
{
  ezJniObject::operator=(other);
  return *this;
}

jclass ezJniClass::GetHandle() const
{
  return static_cast<jclass>(GetJObject());
}

bool ezJniClass::IsAssignableFrom(const ezJniClass& other) const
{
  static bool checkedApiOrder = false;
  static bool reverseArgs = false;

  JNIEnv* env = ezJniAttachment::GetEnv();

  // Guard against JNI bug reversing order of arguments - fixed in
  // https://android.googlesource.com/platform/art/+/1268b742c8cff7318dc0b5b283cbaeabfe0725ba
  if (!checkedApiOrder)
  {
    ezJniClass objectClass("java/lang/Object");
    ezJniClass stringClass("java/lang/String");

    if (env->IsAssignableFrom(jclass(objectClass.GetJObject()), jclass(stringClass.GetJObject())))
    {
      reverseArgs = true;
    }
    checkedApiOrder = true;
  }

  if (!reverseArgs)
  {
    return env->IsAssignableFrom(jclass(other.GetJObject()), jclass(GetJObject()));
  }
  else
  {
    return env->IsAssignableFrom(jclass(GetJObject()), jclass(other.GetJObject()));
  }
}

bool ezJniClass::IsPrimitive()
{
  return UnsafeCall<bool>("isPrimitive", "()Z");
}

ezJniNullPtr::ezJniNullPtr(ezJniClass& clazz)
{
  m_class = clazz;
}

const ezJniString ezJniNullPtr::GetTypeSignature() const
{
  ezJniString jSignature = m_class.UnsafeCall<ezJniString>("getName", "()Ljava/lang/String;");
  ezStringBuilder signature{jSignature.GetData()};
  signature.ReplaceAll(".", "/");
  return ezJniString{signature.GetData()};
}

#endif


