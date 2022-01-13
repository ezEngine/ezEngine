module ez.foundation.math.transform;

public import ez.foundation.math.vec4;
public import ez.foundation.math.quat;

// CODEGEN: SynthesizeTemplate("ezTransformTemplate<TYPE>", "ezTransformTemplate<float>", "ezTransformTemplate<double>")

extern(C++, struct) struct ezTransformTemplate(TYPE)
{
  // CODEGEN-BEGIN: Struct("ezTransformTemplate<TYPE>")
  // Operator: %
  ezVec3Template!(TYPE) m_vPosition;
  ezQuatTemplate!(TYPE) m_qRotation;
  ezVec3Template!(TYPE) m_vScale;
  void SetFromMat4(ref const(ezMat4Template!(TYPE)) mat);
  void SetIdentity();
  static const(ezTransformTemplate!(TYPE)) IdentityTransform();
  TYPE GetMaxScale() const;
  bool ContainsNegativeScale() const;
  bool ContainsUniformScale() const;
  bool IsIdentical(ref const(ezTransformTemplate!(TYPE)) rhs) const;
  bool IsEqual(ref const(ezTransformTemplate!(TYPE)) rhs, TYPE fEpsilon) const;
  void Invert();
  const(ezTransformTemplate!(TYPE)) GetInverse() const;
  ezVec3Template!(TYPE) TransformPosition(ref const(ezVec3Template!(TYPE)) v) const;
  ezVec3Template!(TYPE) TransformDirection(ref const(ezVec3Template!(TYPE)) v) const;
  // Operator: +=
  // Operator: -=
  void SetLocalTransform(ref const(ezTransformTemplate!(TYPE)) GlobalTransformParent, ref const(ezTransformTemplate!(TYPE)) GlobalTransformChild);
  void SetGlobalTransform(ref const(ezTransformTemplate!(TYPE)) GlobalTransformParent, ref const(ezTransformTemplate!(TYPE)) LocalTransformChild);
  const(ezMat4Template!(TYPE)) GetAsMat4() const;
  // Operator: =
  // CODEGEN-END
}

alias ezTransform = ezTransformTemplate!(float);

