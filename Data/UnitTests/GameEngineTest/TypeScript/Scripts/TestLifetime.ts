import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestLifetime extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    step: number = 0;
    obj1: ez.GameObject = null;
    comp1: ez.MeshComponent = null;
    comp2: ez.MeshComponent = null;

    ExecuteTests(): boolean {

        if (this.step == 0) {

            let d = new ez.GameObjectDesc;
            d.Name = "Jonny";
            d.LocalPosition = new ez.Vec3(1, 2, 3);
            d.Dynamic = true;
            d.Parent = this.GetOwner();

            this.obj1 = ez.World.CreateObject(d);

            EZ_TEST.BOOL(this.obj1.GetName() == d.Name);
            EZ_TEST.BOOL(this.obj1.GetParent() == this.GetOwner());
            EZ_TEST.BOOL(this.obj1.GetLocalPosition().IsEqual(d.LocalPosition));
            EZ_TEST.BOOL(this.obj1.GetLocalRotation().IsEqualRotation(ez.Quat.IdentityQuaternion()));
            EZ_TEST.BOOL(this.obj1.GetLocalScaling().IsEqual(ez.Vec3.OneVector()));
            EZ_TEST.FLOAT(this.obj1.GetLocalUniformScaling(), 1.0, 0.0001);

            this.comp1 = ez.World.CreateComponent(this.obj1, ez.MeshComponent);
            this.comp1.Mesh = "{ 6d619c33-6611-432b-a924-27b1b9bfd8db }"; // Box
            this.comp1.Color = ez.Color.BlueViolet();

            this.comp2 = ez.World.CreateComponent(this.obj1, ez.MeshComponent);
            this.comp2.Mesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Sphere
            this.comp2.Color = ez.Color.PaleVioletRed();

            return true;
        }

        if (this.step == 1) {
            EZ_TEST.BOOL(this.obj1 != null);
            EZ_TEST.BOOL(this.obj1.IsValid());

            EZ_TEST.BOOL(this.comp1.IsValid());
            EZ_TEST.BOOL(this.comp2.IsValid());

            this.obj1.SetLocalUniformScaling(2.0);
            
            ez.World.DeleteComponent(this.comp2);
            
            EZ_TEST.BOOL(!this.comp2.IsValid());

            return true;
        }

        if (this.step == 2) {
            EZ_TEST.BOOL(this.obj1 != null);
            EZ_TEST.BOOL(this.obj1.IsValid());

            EZ_TEST.BOOL(this.comp1.IsValid());
            EZ_TEST.BOOL(!this.comp2.IsValid());

            ez.World.DeleteObjectDelayed(this.obj1);

            // still valid this frame
            EZ_TEST.BOOL(this.obj1.IsValid());
            EZ_TEST.BOOL(this.comp1.IsValid());

            return true;
        }

        if (this.step == 3) {
            EZ_TEST.BOOL(this.obj1 != null);
            EZ_TEST.BOOL(!this.obj1.IsValid());
            EZ_TEST.BOOL(!this.comp1.IsValid());
            EZ_TEST.BOOL(!this.comp2.IsValid());
        }

        return false;
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestLifetime") {

            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {
                msg.Message = "done";
            }

            this.step += 1;
        }
    }

}

