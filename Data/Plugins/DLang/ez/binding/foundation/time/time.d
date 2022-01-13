module ez.foundation.time.time;

extern(C++, struct) struct ezTime
{
  // CODEGEN-BEGIN: Struct("ezTime")
  static ezTime Now();
  static ezTime Nanoseconds(double fNanoseconds);
  static ezTime Microseconds(double fMicroseconds);
  static ezTime Milliseconds(double fMilliseconds);
  static ezTime Seconds(double fSeconds);
  static ezTime Minutes(double fMinutes);
  static ezTime Hours(double fHours);
  static ezTime Zero();
  // Operator: %
  void SetZero();
  bool IsZero() const;
  bool IsNegative() const;
  bool IsPositive() const;
  bool IsZeroOrNegative() const;
  bool IsZeroOrPositive() const;
  float AsFloatInSeconds() const;
  double GetNanoseconds() const;
  double GetMicroseconds() const;
  double GetMilliseconds() const;
  double GetSeconds() const;
  double GetMinutes() const;
  double GetHours() const;
  // Operator: -=
  // Operator: +=
  // Operator: *=
  // Operator: /=
  // Operator: -
  // Operator: +
  // Operator: -
  // Operator: <
  // Operator: <=
  // Operator: >
  // Operator: >=
  // Operator: ==
  // Operator: !=
private:
  double m_fTime;
  // Operator: =
  // CODEGEN-END
}
