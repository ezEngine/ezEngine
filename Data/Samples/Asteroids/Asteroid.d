import ez;
import std.math.trigonometry;
import std.string;
import ez.foundation.strings.formatstring;
import ez.foundation.math.color;

extern(C++, class) struct Test
{
    extern export static __gshared const(int*) const_vars;
    extern export static __gshared int[2] nonconst_vars;
}

extern(C++) struct SmallStruct
{
  float value = 0;
//   float value2 = 0;
//   float value3 = 0;

  static SmallStruct GetValue(float input);
};

extern (C++) class AsteroidsTestComponent : ezDLangComponent
{
    mixin ExposeToCpp;

    override void OnSimulationStarted()
    {
        ezLog.Info("OnSimulationStarted()\n");
    }

    override void Update()
    {
        //SmallStruct s = SmallStruct.GetValue(3);

        ezAngle angle = ezAngle.Degree(45);
        ezAngle angle2 = ezAngle.Degree(45);

        if (angle == angle2)
        {
          ezLog.Info("angles are equal");
        }
        else 
        {
          ezLog.Info("angles are not equal");
        }

        angle += angle2;

        if (angle < angle2)
        {
          ezLog.Info("angle < angle2");
        }
        else 
        {
          ezLog.Info("angle >= angle2");
        }

        ezLog.Info(format("Angle: %s", (angle + angle2).GetDegree()));
        //ezLog.Info(format("value: %f", s.value));

        ezComponent owner = GetOwnerComponent();

        if (owner.IsActive())
        {
          ezLog.Info("Component name: " ~ CstrToDstr(owner.GetOwner().GetName()));

          const ezRTTI* rtti  = owner.GetDynamicRTTI();
          
          ezLog.Info("Component type: " ~ CstrToDstr(rtti.GetTypeName()));
        }

        for (const (ezRTTI)* rtti = ezRTTI.GetFirstInstance(); rtti != null; rtti = rtti.GetNextInstance())
        {
          ezLog.Info("RTTI type: " ~ CstrToDstr(rtti.GetTypeName()));
          break;
        }
    }
}
