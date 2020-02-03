import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestComponent extends ez.TypescriptComponent {

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

        let mesh = owner.TryGetComponentOfBaseType(ez.MeshComponent);
        let text = owner.TryGetComponentOfBaseType(ez.DebugTextComponent);

        // IsValid
        {
            EZ_TEST.BOOL(mesh != null && mesh.IsValid());
            EZ_TEST.BOOL(text != null && text.IsValid());
        }

        // GetOWner
        {
            EZ_TEST.BOOL(mesh.GetOwner() == owner);
            EZ_TEST.BOOL(text.GetOwner() == owner);
        }

        // Active Flag / Active State
        {
            EZ_TEST.BOOL(mesh.IsActive());
            EZ_TEST.BOOL(mesh.IsActiveAndInitialized());
            EZ_TEST.BOOL(mesh.IsActiveAndSimulating());
            
            EZ_TEST.BOOL(!text.GetActiveFlag());
            EZ_TEST.BOOL(!text.IsActive());
            EZ_TEST.BOOL(!text.IsActiveAndInitialized());
            
            text.SetActiveFlag(true);
            EZ_TEST.BOOL(text.GetActiveFlag());
            EZ_TEST.BOOL(text.IsActive());
            EZ_TEST.BOOL(text.IsActiveAndInitialized());
            
            mesh.SetActiveFlag(false);
            EZ_TEST.BOOL(!mesh.GetActiveFlag());
            EZ_TEST.BOOL(!mesh.IsActive());
            EZ_TEST.BOOL(!mesh.IsActiveAndInitialized());
            EZ_TEST.BOOL(!mesh.IsActiveAndSimulating());
        }

        // GetUniqueID
        {
            // ezInvalidIndex
            EZ_TEST.INT(mesh.GetUniqueID(), 4294967295);
            EZ_TEST.INT(text.GetUniqueID(), 4294967295);
        }

        // TryGetScriptComponent
        {
            let sc = this.GetOwner().TryGetScriptComponent("TestComponent");

            EZ_TEST.BOOL(sc == this);
        }

        // interact with C++ components
        {
            let c = ez.World.CreateComponent(this.GetOwner(), ez.MoveToComponent);

            // execute function
            c.SetTargetPosition(new ez.Vec3(1, 2, 3));

            // get/set properties
            c.TranslationSpeed = 23;
            EZ_TEST.FLOAT(c.TranslationSpeed, 23);
            
            c.TranslationSpeed = 17;
            EZ_TEST.FLOAT(c.TranslationSpeed, 17);
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestComponent") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

