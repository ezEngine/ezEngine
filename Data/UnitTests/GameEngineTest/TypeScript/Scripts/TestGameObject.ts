import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestGameObject extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        let owner = this.GetOwner();
        let child1 = owner.FindChildByName("Child1");
        let child2 = owner.FindChildByPath("Child2");


        // IsValid
        {
            EZ_TEST.BOOL(owner.IsValid());
            EZ_TEST.BOOL(child1.IsValid());
            EZ_TEST.BOOL(child2.IsValid());
        }

        // GetName / SetName
        {
            EZ_TEST.BOOL(owner.GetName() == "GameObject");
            owner.SetName("TestGameObject")
            EZ_TEST.BOOL(owner.GetName() == "TestGameObject");
        }

        // Active
        {
            EZ_TEST.BOOL(child1.IsActive());
            EZ_TEST.BOOL(child2.IsActive());

            child2.SetActive(false);

            EZ_TEST.BOOL(child1.IsActive());
            EZ_TEST.BOOL(!child2.IsActive());

            child2.SetActive(true);

            EZ_TEST.BOOL(child1.IsActive());
            EZ_TEST.BOOL(child2.IsActive());
        }

        // Local Position
        {
            EZ_TEST.VEC3(child1.GetLocalPosition(), new ez.Vec3(1, 2, 3));
            EZ_TEST.VEC3(child2.GetLocalPosition(), new ez.Vec3(4, 5, 6));

            child1.SetLocalPosition(new ez.Vec3(11, 22, 33));
            EZ_TEST.VEC3(child1.GetLocalPosition(), new ez.Vec3(11, 22, 33));
        }

        // Local Rotation
        {
            EZ_TEST.QUAT(child1.GetLocalRotation(), ez.Quat.IdentityQuaternion());

            let nr = new ez.Quat();
            nr.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(45));

            child1.SetLocalRotation(nr);
            EZ_TEST.QUAT(child1.GetLocalRotation(), nr);
        }

        // Local Scaling
        {
            EZ_TEST.VEC3(child2.GetLocalScaling(), new ez.Vec3(2, 3, 4));
            EZ_TEST.FLOAT(child2.GetLocalUniformScaling(), 5);

            child2.SetLocalScaling(new ez.Vec3(22, 33, 44));
            child2.SetLocalUniformScaling(55);

            EZ_TEST.VEC3(child2.GetLocalScaling(), new ez.Vec3(22, 33, 44));
            EZ_TEST.FLOAT(child2.GetLocalUniformScaling(), 55);
        }

        // Global Position
        {
            EZ_TEST.VEC3(child1.GetGlobalPosition(), new ez.Vec3(1, 2, 3));
            EZ_TEST.VEC3(child2.GetGlobalPosition(), new ez.Vec3(4, 5, 6));

            child1.SetGlobalPosition(new ez.Vec3(11, 22, 33));
            EZ_TEST.VEC3(child1.GetGlobalPosition(), new ez.Vec3(11, 22, 33));

        }

        // Global Rotation
        {
            EZ_TEST.QUAT(child1.GetGlobalRotation(), ez.Quat.IdentityQuaternion());

            let nr = new ez.Quat();
            nr.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(30));

            child1.SetGlobalRotation(nr);
            EZ_TEST.QUAT(child1.GetGlobalRotation(), nr);
        }

        // Global Scaling
        {
            EZ_TEST.VEC3(child2.GetGlobalScaling(), new ez.Vec3(2 * 5, 3 * 5, 4 * 5));

            child2.SetGlobalScaling(new ez.Vec3(1, 2, 3));

            EZ_TEST.VEC3(child2.GetGlobalScaling(), new ez.Vec3(1, 2, 3));
            EZ_TEST.FLOAT(child2.GetLocalUniformScaling(), 1);
        }

        // Global Dirs
        {
            child1.SetGlobalRotation(ez.Quat.IdentityQuaternion());

            EZ_TEST.VEC3(child1.GetGlobalDirForwards(), new ez.Vec3(1, 0, 0));
            EZ_TEST.VEC3(child1.GetGlobalDirRight(), new ez.Vec3(0, 1, 0));
            EZ_TEST.VEC3(child1.GetGlobalDirUp(), new ez.Vec3(0, 0, 1));

            let r = new ez.Quat();
            r.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

            child1.SetGlobalRotation(r);

            EZ_TEST.VEC3(child1.GetGlobalDirForwards(), new ez.Vec3(0, 1, 0));
            EZ_TEST.VEC3(child1.GetGlobalDirRight(), new ez.Vec3(-1, 0, 0));
            EZ_TEST.VEC3(child1.GetGlobalDirUp(), new ez.Vec3(0, 0, 1));
        }

        // Velocity
        {
            EZ_TEST.VEC3(child1.GetVelocity(), ez.Vec3.ZeroVector());

            child1.SetVelocity(new ez.Vec3(1, 2, 3));
            EZ_TEST.VEC3(child1.GetVelocity(), new ez.Vec3(1, 2, 3));
        }

        // Team ID
        {
            EZ_TEST.FLOAT(child1.GetTeamID(), 0);
            child1.SetTeamID(11);
            EZ_TEST.FLOAT(child1.GetTeamID(), 11);
        }

        // FindChildByName
        {
            let c = owner.FindChildByName("Child1_Child1", false);
            EZ_TEST.BOOL(c == null);

            c = owner.FindChildByName("Child1_Child1", true);
            EZ_TEST.BOOL(c != null);
            EZ_TEST.BOOL(c.IsValid());
            EZ_TEST.BOOL(c.GetName() == "Child1_Child1");
        }

        // FindChildByName
        {
            let c = owner.FindChildByPath("Child2_Child1");
            EZ_TEST.BOOL(c == null);

            c = owner.FindChildByPath("Child2/Child2_Child1");
            EZ_TEST.BOOL(c != null);
            EZ_TEST.BOOL(c.IsValid());
            EZ_TEST.BOOL(c.GetName() == "Child2_Child1");
        }

        // SearchForChildByNameSequence
        {
            let c = owner.SearchForChildByNameSequence("Child1_Child1/A");
            EZ_TEST.BOOL(c != null && c.IsValid());
            EZ_TEST.FLOAT(c.GetLocalUniformScaling(), 2);

            c = owner.SearchForChildWithComponentByNameSequence("Child2/A", ez.PointLightComponent);
            EZ_TEST.BOOL(c != null && c.IsValid());
            EZ_TEST.FLOAT(c.GetLocalUniformScaling(), 3);
        }

        // TryGetComponentOfBaseType
        {
            let sl = child1.TryGetComponentOfBaseType(ez.SpotLightComponent);
            EZ_TEST.BOOL(sl != null && sl.IsValid());

            let pl = child1.TryGetComponentOfBaseTypeName<ez.SpotLightComponent>("ezPointLightComponent");
            EZ_TEST.BOOL(pl != null && pl.IsValid());
        }

        // Tags
        {
            EZ_TEST.BOOL(owner.HasAllTags("AutoColMesh"));
            EZ_TEST.BOOL(owner.HasAllTags("CastShadow"));
            EZ_TEST.BOOL(owner.HasAllTags("AutoColMesh", "CastShadow"));
            EZ_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "NOTAG"));
            EZ_TEST.BOOL(owner.HasAnyTags("CastShadow", "NOTAG"));
            EZ_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "CastShadow"));
            
            owner.RemoveTags("CastShadow", "AutoColMesh");
            EZ_TEST.BOOL(!owner.HasAnyTags("AutoColMesh", "CastShadow"));
            
            owner.AddTags("CastShadow", "TAG1");
            EZ_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "CastShadow"));
            EZ_TEST.BOOL(!owner.HasAllTags("AutoColMesh", "CastShadow"));
            
            owner.SetTags("TAG");
            EZ_TEST.BOOL(owner.HasAnyTags("TAG"));
            EZ_TEST.BOOL(!owner.HasAnyTags("AutoColMesh", "CastShadow", "TAG1"));
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestGameObject") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

