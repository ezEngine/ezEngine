#pragma once

#include <Core/CoreDLL.h>

#include <Core/Scripting/ScriptRTTI.h>

class ezScriptWorldModule;

using ezScriptCoroutineId = ezGenericId<20, 12>;

/// \brief A handle to a script coroutine which can be used to determine whether a coroutine is still running
/// even after the underlying coroutine object has already been deleted.
///
/// \sa ezScriptWorldModule::CreateCoroutine, ezScriptWorldModule::IsCoroutineFinished
struct ezScriptCoroutineHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezScriptCoroutineHandle, ezScriptCoroutineId);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptCoroutineHandle);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezScriptCoroutineHandle);

/// \brief Base class of script coroutines.
///
/// A coroutine is a function that can be distributed over multiple frames and behaves similar to a mini state machine.
/// That is why coroutines are actually individual objects that keep track of their state rather than simple functions.
/// At first Start() is called with the arguments of the coroutine followed by one or multiple calls to Update().
/// The return value of the Update() function determines whether the Update() function should be called again next frame
/// or at latest after the specified delay. If the Update() function returns completed the Stop() function is called and the
/// coroutine object is destroyed.
/// The ezScriptWorldModule is used to create and manage coroutine objects. The coroutine can then either be started and
/// scheduled automatically by calling ezScriptWorldModule::StartCoroutine or the
/// Start/Stop/Update function is called manually if the coroutine is embedded as a subroutine in another coroutine.
class EZ_CORE_DLL ezScriptCoroutine
{
public:
  ezScriptCoroutine();
  virtual ~ezScriptCoroutine();

  ezScriptCoroutineHandle GetHandle() { return ezScriptCoroutineHandle(m_Id); }

  ezStringView GetName() const { return m_sName; }

  ezScriptInstance* GetScriptInstance() { return m_pInstance; }
  const ezScriptInstance* GetScriptInstance() const { return m_pInstance; }

  ezScriptWorldModule* GetScriptWorldModule() { return m_pOwnerModule; }
  const ezScriptWorldModule* GetScriptWorldModule() const { return m_pOwnerModule; }

  struct Result
  {
    struct State
    {
      using StorageType = ezUInt8;

      enum Enum
      {
        Invalid,
        Running,
        Completed,
        Failed,

        Default = Invalid,
      };
    };

    static EZ_ALWAYS_INLINE Result Running(ezTime maxDelay = ezTime::MakeZero()) { return {State::Running, maxDelay}; }
    static EZ_ALWAYS_INLINE Result Completed() { return {State::Completed}; }
    static EZ_ALWAYS_INLINE Result Failed() { return {State::Failed}; }

    ezEnum<State> m_State;
    ezTime m_MaxDelay = ezTime::MakeZero();
  };

  virtual void StartWithVarargs(ezArrayPtr<ezVariant> arguments) = 0;
  virtual void Stop() {}
  virtual Result Update(ezTime deltaTimeSinceLastUpdate) = 0;

  void UpdateAndSchedule(ezTime deltaTimeSinceLastUpdate = ezTime::MakeZero());

private:
  friend class ezScriptWorldModule;
  void Initialize(ezScriptCoroutineId id, ezStringView sName, ezScriptInstance& inout_instance, ezScriptWorldModule& inout_ownerModule);
  void Deinitialize();

  static const ezAbstractFunctionProperty* GetUpdateFunctionProperty();

  ezScriptCoroutineId m_Id;
  ezHashedString m_sName;
  ezScriptInstance* m_pInstance = nullptr;
  ezScriptWorldModule* m_pOwnerModule = nullptr;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptCoroutine);

/// \brief Base class of coroutines which are implemented in C++ to allow automatic unpacking of the arguments from variants
template <typename Derived, class... Args>
class ezTypedScriptCoroutine : public ezScriptCoroutine
{
private:
  template <std::size_t... I>
  EZ_ALWAYS_INLINE void StartImpl(ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>)
  {
    static_cast<Derived*>(this)->Start(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void StartWithVarargs(ezArrayPtr<ezVariant> arguments) override
  {
    StartImpl(arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};

/// \brief Mode that decides what should happen if a new coroutine is created while there is already another coroutine running with the same name
/// on a given instance.
///
/// \sa ezScriptWorldModule::CreateCoroutine
struct ezScriptCoroutineCreationMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    StopOther,     ///< Stop the other coroutine before creating a new one with the same name
    DontCreateNew, ///< Don't create a new coroutine if there is already one running with the same name
    AllowOverlap,  ///< Allow multiple overlapping coroutines with the same name

    Default = StopOther
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptCoroutineCreationMode);

/// \brief A coroutine type that stores a custom allocator.
///
/// The custom allocator allows to pass more data to the created coroutine object than the default allocator.
/// E.g. this is used to pass the visual script graph to a visual script coroutine without the user needing to know
/// that the coroutine is actually implemented in visual script.
class EZ_CORE_DLL ezScriptCoroutineRTTI : public ezRTTI, public ezRefCountingImpl
{
public:
  ezScriptCoroutineRTTI(ezStringView sName, ezUniquePtr<ezRTTIAllocator>&& pAllocator);
  ~ezScriptCoroutineRTTI();

private:
  ezString m_sTypeNameStorage;
  ezUniquePtr<ezRTTIAllocator> m_pAllocatorStorage;
};

/// \brief A function property that creates an instance of the given coroutine type and starts it immediately.
class EZ_CORE_DLL ezScriptCoroutineFunctionProperty : public ezScriptFunctionProperty
{
public:
  ezScriptCoroutineFunctionProperty(ezStringView sName, const ezSharedPtr<ezScriptCoroutineRTTI>& pType, ezScriptCoroutineCreationMode::Enum creationMode);
  ~ezScriptCoroutineFunctionProperty();

  virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::Member; }
  virtual const ezRTTI* GetReturnType() const override { return nullptr; }
  virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override { return ezPropertyFlags::Void; }
  virtual ezUInt32 GetArgumentCount() const override { return 0; }

  virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override
  {
    EZ_IGNORE_UNUSED(uiParamIndex);
    return nullptr;
  }

  virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override
  {
    EZ_IGNORE_UNUSED(uiParamIndex);
    return ezPropertyFlags::Void;
  }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& out_returnValue) const override;

protected:
  ezSharedPtr<ezScriptCoroutineRTTI> m_pType;
  ezEnum<ezScriptCoroutineCreationMode> m_CreationMode;
};

/// \brief A message handler that creates an instance of the given coroutine type and starts it immediately.
class EZ_CORE_DLL ezScriptCoroutineMessageHandler : public ezScriptMessageHandler
{
public:
  ezScriptCoroutineMessageHandler(ezStringView sName, const ezScriptMessageDesc& desc, const ezSharedPtr<ezScriptCoroutineRTTI>& pType, ezScriptCoroutineCreationMode::Enum creationMode);
  ~ezScriptCoroutineMessageHandler();

  static void Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg);

protected:
  ezHashedString m_sName;
  ezSharedPtr<ezScriptCoroutineRTTI> m_pType;
  ezEnum<ezScriptCoroutineCreationMode> m_CreationMode;
};

/// \brief HashHelper implementation so coroutine handles can be used as key in a hash table. Also needed to store in a variant.
template <>
struct ezHashHelper<ezScriptCoroutineHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezScriptCoroutineHandle value) { return ezHashHelper<ezUInt32>::Hash(value.GetInternalID().m_Data); }

  EZ_ALWAYS_INLINE static bool Equal(ezScriptCoroutineHandle a, ezScriptCoroutineHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for coroutine handles.
EZ_ALWAYS_INLINE void operator<<(ezStreamWriter& inout_stream, const ezScriptCoroutineHandle& hValue)
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(hValue);
  EZ_ASSERT_NOT_IMPLEMENTED;
}

EZ_ALWAYS_INLINE void operator>>(ezStreamReader& inout_stream, ezScriptCoroutineHandle& ref_hValue)
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(ref_hValue);
  EZ_ASSERT_NOT_IMPLEMENTED;
}
