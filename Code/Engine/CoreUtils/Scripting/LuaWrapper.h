#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>

extern "C"
{
  #include <ThirdParty/Lua/lua.h>
  #include <ThirdParty/Lua/lualib.h>
  #include <ThirdParty/Lua/lauxlib.h>
}

/// This class encapsulates ONE Lua-Script.
///
///It makes it easier to interact with the script, to get data
/// out of it (e.g. for configuration files), to register C-functions to it and to call script-functions. It is possible
/// to load more than one Lua-File into one Lua-Script, one can dynamically generate code and pass it as
/// a string to the script.
/// It ALSO allows to construct the ezLuaWrapper with a working lua_State-Pointer and thus
/// only simplify interaction with an already existing script (for example, when a C-Function is called in a Script,
/// it passes its lua_State to that Function).
///
/// \note Lua starts counting at 1, not at 0. However ezLuaWrapper does NOT do this, but uses the C++ convention instead! 
/// That means, when you query the first parameter or return-value passed to your function, you need to query for value 0, not for value 1.
class EZ_COREUTILS_DLL ezLuaWrapper
{
public:

  /// \name Setting up the Script
  /// @{ 

  /// Generates a NEW Lua-Script, which is empty.
  ezLuaWrapper(); // [tested]

  /// Takes an EXISTING Lua-Script and allows to get easier access to it.
  ezLuaWrapper(lua_State* s);

  /// Destroys the Lua-Script, if it was created, but leaves it intact, if this instance did not generate the Lua-Script.
  ~ezLuaWrapper(); // [tested]

  /// Clears the script to be empty.
  void Clear(); // [tested]

  /// Returns the lua state for custom access.
  ///
  /// It is not recommended to modify the lua state directly, however for certain functionality that the wrapper does not implement
  /// this might be necessary. Make sure to either do all modifications at the start, before using the LuaWrapper on it (as it has some
  /// internal state), or to only do actions that will end up in the same stack state as before.
  lua_State* GetLuaState();

  /// Executes a string containing Lua-Code.
  ///
  /// \param szString
  ///   The lua code to execute.
  /// \param szDebugChunkName
  ///   An optional name for the lua code, to ease debugging when errors occur.
  /// \param pLogInterface
  ///   An optional log interface where error messages are written to. If nullptr is passed in, error messages are written to the global log.
  ezResult ExecuteString(const char* szString, const char* szDebugChunkName = "chunk", ezLogInterface* pLogInterface = nullptr) const; // [tested]

  /// @}


  /// \name Managing Tables
  /// @{ 

  /// Opens the Lua-Table with the given name for reading and writing.
  ///
  /// All following calls to functions that read/write variables are working in the scope of the last opened table.
  /// The table to open needs to be in scope itself. Returns EZ_FAILURE, if it's not possible (the table does not exist in this scope).
  ezResult OpenTable(const char* szTable); // [tested]

  /// Opens the Table n, that was passed to a C-Function on its Parameter-Stack.
  ezResult OpenTableFromParameter(ezUInt32 iFunctionParameter); // [tested]

  /// Closes the table that was opened last.
  void CloseTable(); // [tested]

  /// Closes all open Tables.
  void CloseAllTables(); // [tested]

  /// Pushes an existing table onto Lua's stack. Either one that is in local scope, or a global table.
  void PushTable(const char* szTableName, bool bGlobalTable); // [tested]

  /// @}

  /// \name Variable and Function Checks
  /// @{ 

  /// Checks, whether the Variable with the given Name exists.
  bool IsVariableAvailable(const char* szVariable) const; // [tested]

  /// Checks, whether the Function with the given Name exists.
  bool IsFunctionAvailable(const char* szFunction) const; // [tested]

  /// @}

  /// \name Reading Variables
  /// @{ 

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  int GetIntVariable(const char* szName, ezInt32 iDefault = 0) const; // [tested]

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  bool GetBoolVariable(const char* szName, bool bDefault = false) const; // [tested]

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  float GetFloatVariable(const char* szName, float fDefault = 0.0f) const; // [tested]

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  const char* GetStringVariable(const char* szName, const char* szDefault="") const; // [tested]

  /// @}

  /// \name Modifying Variables
  /// @{ 

  /// Sets the Variable with the given name (in scope) to nil.
  void SetVariableNil(const char* szName) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, ezInt32 iValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, bool bValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, float fValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, const char* szValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, const char* szValue, ezUInt32 len) const; // [tested]

  /// @}

  /// \name Calling Functions
  /// @{ 

  /// Registers a C-Function to the Script under a certain Name.
  void RegisterCFunction(const char* szFunctionName, lua_CFunction pFunction, void* pLightUserData = nullptr) const; // [tested]

  /// Prepares a function to be called. After that the parameters can be pushed. Returns false if no function with the given name exists in the scope.
  bool PrepareFunctionCall(const char* szFunctionName); // [tested]

  /// Calls the prepared Function with the previously pushed Parameters.
  ///
  /// You must pass in how many return values you expect from this function and the function must stick to that, otherwise an assert will trigger.
  /// After you are finished inspecting the return values, you need to call DiscardReturnValues() to clean them up.
  ///
  /// Returns EZ_FAILURE if anything went wrong during function execution. Reports errors via \a pLogInterface.
  ezResult CallPreparedFunction(ezUInt32 iExpectedReturnValues = 0, ezLogInterface* pLogInterface = nullptr); // [tested]

  /// Call this after you called a prepared Lua-function, that returned some values. If zero values were returned, this function is optional.
  void DiscardReturnValues(); // [tested]

  /// Return the value of this function in a called C-Function.
  ///
  /// A typical C function should look like this:
  /// \code
  /// int CFunction(lua_State* state)
  /// {
  ///   ezLuaWrapper s(state);
  ///   .. do something ..
  ///   return s.ReturnToScript();
  /// }
  /// \endcode
  ezInt32 ReturnToScript() const; // [tested]

  /// @}

  /// \name Calling Function with Parameters
  /// @{ 

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter (ezInt32 iParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter (bool bParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter (float fParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter (const char* szParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter (const char* szParam, ezUInt32 length); // [tested]

  /// Pushes a nil parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameterNil(); // [tested]

  /// @}

  /// \name Inspecting Function Parameters
  /// @{ 

  /// \brief Returns the currently executed function light user data that was passed to RegisterCFunction.
  void* GetFunctionLightUserData() const;

  /// Returns how many Parameters were passed to the called C-Function.
  ezUInt32 GetNumberOfFunctionParameters() const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterInt(ezUInt32 iParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterBool(ezUInt32 iParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterFloat(ezUInt32 iParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterTable(ezUInt32 iParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterString(ezUInt32 iParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterNil(ezUInt32 iParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  int GetIntParameter (ezUInt32 iParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  bool GetBoolParameter (ezUInt32 iParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  float GetFloatParameter (ezUInt32 iParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  const char* GetStringParameter (ezUInt32 iParameter) const; // [tested]

  /// @}

  /// \name Function Return Values
  /// @{ 

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(ezInt32 iParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(bool bParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(float fParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(const char* szParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(const char* szParam, ezUInt32 length); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValueNil(); // [tested]


  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueInt(ezUInt32 iReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueBool(ezUInt32 iReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueFloat(ezUInt32 iReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueString(ezUInt32 iReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueNil(ezUInt32 iReturnValue) const; // [tested]


  /// Returns the value of the nth return-value.
  int GetIntReturnValue(ezUInt32 iReturnValue) const; // [tested]

  /// Returns the value of the nth return-value.
  bool GetBoolReturnValue(ezUInt32 iReturnValue) const; // [tested]

  /// Returns the value of the nth return-value.
  float GetFloatReturnValue(ezUInt32 iReturnValue) const; // [tested]

  /// Returns the value of the nth return-value.
  const char* GetStringReturnValue(ezUInt32 iReturnValue) const; // [tested]

  /// @}


private:
  /// An Allocator for Lua. Optimizes (in theory) the allocations.
  static void* lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize);

  /// The Lua-State for the Script.
  lua_State* m_pState;

  /// If this script created the Lua-State, it also releases it on exit.
  bool m_bReleaseOnExit;

  struct ezScriptStates
  {
    ezScriptStates() : m_iParametersPushed (0), m_iOpenTables (0), m_iLuaReturnValues (0) {}

    /// How many Parameters were pushed for the next function-call.
    ezInt32 m_iParametersPushed;

    /// How many Tables have been opened inside the Lua-Script.
    ezInt32 m_iOpenTables;

    /// How many values the called lua-function should return
    ezInt32 m_iLuaReturnValues;
  };

  ezScriptStates m_States;

  static const ezInt32 s_ParamOffset = 1; // should be one, to start counting at 0, instead of 1
};

#include <CoreUtils/Scripting/LuaWrapper/LuaWrapper.inl>



