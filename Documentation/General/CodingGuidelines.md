Coding Guidelines {#CodingGuidelines}
=================

Look at the existing code to get a general feeling for the coding style.

  * Functions start with an upper case letter.
  * Classes start with the engine prefix (ez).
  * Curly braces on their own lines.
  * Macros / `#define`s in `UPPER_CASE`
  * No `#include` guards, use `#pragma` once instead
  * No space between function calls and the parameter list.
  * Use spaces between operators, to make such code more readable.
  * Do not try to put as much code into one line, as possible.
  * Use temporary variables to document what's going on.
  * Use `const` whereever possible.
  * `m_` for member variables.
  * `s_` for static variables.
  * `g_` for global variables.
  * `operator=` should always return void, instead of `*this`
 
  * Simple hungarian notation:
    * `i` for all signed int types
    * `ui` for all unsigned int types
    * `f` for floats and doubles
    * `sz` for zero-terminated strings
    * `s` for all other string types
    * `p` for all pointers, including pointers-to-pointers, etc.
    * `b` for bool
    * `v` for vectors.
    * Everything else, as you wish (so usually no prefix).
  * Member variables that have no prefix should start with an upper-case letter. E.g. `m_MyVar`
  * Local variables that have no prefix should start with an lower-case letter. E.g. `angle`
    
  * Use empty lines and good formatting to make the code more readable.
  * Write unit-tests for (nearly) everything.
    * Mark functions with `[tested]` to indicate that a unit-test exists for that function.
    * Mark functions with `\\test` to indicate that a test is needed but not yet written.
  * Use Doxygen comments to document your code.
    * All doxygen comments start with three slashes.
    * \\brief `Short Description` for a short description.
    * Followed by one empty comment line (started with three slashes), then more comments for a detailed description.
    * Use \\a to highlight the following word. This can be used to indicate that the following word means one of the function parameters.
    * \\param `Name` to document a specific parameter. This is usually not used, only if one specific parameter needs special treatment.
    * \\todo `Stuff` anywhere in the code, to add a todo item to doxygens todo list.
    * \\test `Stuff` anywhere in the code, to mark that this needs to be tested.
    * \\bug `Stuff` anywhere in the code, to document a known bug.
  
Everything else is up to your judgement.
