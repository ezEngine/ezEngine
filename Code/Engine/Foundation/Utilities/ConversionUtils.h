#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

/// \brief This class contains functions to convert between different types.
///
/// Contains helper functions to convert from strings to numerical values.
/// To convert from numerical values to strings, use ezStringUtils::Format, which provides a rich set of formatting options.
class EZ_FOUNDATION_DLL ezConversionUtils
{
public:

  /// \brief Parses szString and converts it to an integer value. Returns EZ_FAILURE if the string contains no parsable integer value.
  ///
  /// \param szString
  ///   If szString is NULL or an empty string or starts with an some non-whitespace and non-sign character, EZ_FAILURE is returned.
  ///   All whitespaces at the start of the string are skipped. Each minus sign flips the sign of the output value, so --3 will return 3.
  ///   Plus signs are skipped and have no effect, so -+3 will still return -3.
  /// \param out_Res
  ///   The parsed value is returned in out_Res on success. On failure out_Res is not modified at all, so you can store a default value
  ///   in it, before calling StringToInt() and always use that value, even without checking for success or failure.
  /// \param out_LastParsePosition
  ///   On success out_LastParsePosition will contain the address of the character in szString that stopped the parser. This might point
  ///   to the zero terminator of szString, or to some non-digit character, since for example "5+6" will parse as '5' and '+' will be the
  ///   position where the parser stopped (returning EZ_SUCCESS).
  ///   If szString is supposed to only contain one full integer and nothing else, then out_LastParsePosition should alwas point to a zero
  ///   terminator (otherwise the string was malformed).
  /// \return
  ///   EZ_SUCCESS if any integer value could get properly extracted from szString (including 0). This includes that only some part of the
  ///   string was parsed until a non-digit character was encountered.
  ///   EZ_FAILURE if the string starts with something that can not be interpreted as an integer.
  static ezResult StringToInt(const char* szString, ezInt32& out_Res, const char** out_LastParsePosition = NULL); // [tested]

  /// \brief Pases szString and converts it to a doule value. Returns EZ_FAILURE if the string contains no parsable floating point value.
  ///
  /// \param szString
  ///   If szString is NULL or an empty string or starts with an some non-whitespace and non-sign character, EZ_FAILURE is returned.
  ///   All whitespaces at the start of the string are skipped. Each minus sign flips the sign of the output value, so --3 will return 3.
  ///   Plus signs are skipped and have no effect, so -+3 will still return -3.
  ///   The value string may contain one '.' to separate integer and fractional part. It may also contain an 'e' for scientific notation
  ///   followed by an integer exponent value. No '.' may follow after an 'e' anymore.
  ///   Additionally the value may be terminated by an 'f' to indicate a floating point value. The 'f' will be skipped (as can be observed
  ///   through out_LastParsePosition, and it will terminate further parsing, but it will not affect the precision of the result.
  ///   Commas (',') are never treated as fractional part separators (as in the German locale).
  /// \param out_Res
  ///   If EZ_SUCCESS is returned, out_Res will contain the result. Otherwise it stays unmodified.
  ///   The result may have rounding errors, ie. even though a double may be able to represent the string value exactly, there is no guarantee
  ///   that out_Res will be 100% identical. If that is required, use the C lib atof() function.
  /// \param out_LastParsePosition
  ///   On success out_LastParsePosition will contain the address of the character in szString that stopped the parser. This might point
  ///   to the zero terminator of szString, or to some unexpected character, since for example "5+6" will parse as '5' and '+' will be the
  ///   position where the parser stopped (returning EZ_SUCCESS).
  ///   If you want to parse strictly (e.g. you do not want to parse "5+6" as "5") you can use this result to check that only certain characters
  ///   were encountered after the float (e.g. only '\0' or ',').
  /// \return
  ///   EZ_SUCCESS if any text was encountered that can be interpreted as a floating point value.
  ///   EZ_FAILURE otherwise.
  ///
  /// \note
  ///   The difference to the C lib atof() function is that this function properly returns whether something could get parsed as a floating point
  ///   value, at all. atof() just returns zero in such a case. Also the way whitespaces and signs at the beginning of the string are handled is
  ///   different and StringToFloat will return 'success' if it finds anything that can be parsed as a float, even if the string continues with
  ///   invalid text. So you can parse "2.54f+3.5" as "2.54f" and out_LastParsePosition will tell you where the parser stopped.
  ///   On the down-side StringToFloat() is probably not as precise as atof(), because of a very simplistic conversion algorithm.
  ///   If you require the features of StringToFloat() and the precision of atof(), you can let StringToFloat() handle the cases for detecting
  ///   the validity, the sign and where the value ends and then use atof to parse only that substring with maximum precision.
  static ezResult StringToFloat(const char* szString, double& out_Res, const char** out_LastParsePosition = NULL);

  /// \brief Parses szString and checks that the first word it finds stats with a phrase that can be interpreted as a boolean value.
  ///
  /// \param szString
  ///   If szString starts with whitespace characters, they are skipped. EZ_SUCCESS is returned (and out_Res is filled with true/false),
  ///   if the string then starts with any of the following phrases: "true", "false", "on", "off", "yes", "no", "1", "0", "enable", "disable".
  ///   EZ_FAILURE is returned if none of those is encountered (or the string is empty). It does not matter, whether the string continues
  ///   with some other text, e.g. "nolf" is still interpreted as "no". That means you can pass strings such as "true, a = false" into this
  ///   function to just parse the next piece of a command line.
  /// \param out_Res
  ///   If EZ_SUCCESS is returned, out_Res contains a valid value. Otherwise it is not modified. That means you can initialize it with a default
  ///   value that can be used even if EZ_FAILURE is returned.
  /// \param out_LastParsePosition
  ///   On success out_LastParsePosition will contain the address of the character in szString that stopped the parser. This might point
  ///   to the zero terminator of szString, or to the next character after the phrase "true", "false", "on", "off", etc. which was interpreted
  ///   as a boolean value.
  ///   If you want to parse strictly (e.g. you do not want to parse "Nolf" as "no") you can use this result to check that only certain characters
  ///   were encountered after the boolean phrase (such as '\0' or ',' etc.).
  /// \return
  ///   EZ_SUCCESS if any phrase was encountered that can be interpreted as a boolean value.
  ///   EZ_FAILURE otherwise.
  static ezResult StringToBool(const char* szString, bool& out_Res, const char** out_LastParsePosition = NULL); // [tested]


};


