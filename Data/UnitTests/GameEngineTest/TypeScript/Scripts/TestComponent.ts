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

        // Active
        {
            EZ_TEST.BOOL(mesh.IsActive());
            EZ_TEST.BOOL(mesh.IsActiveAndInitialized());
            EZ_TEST.BOOL(mesh.IsActiveAndSimulating());
            
            EZ_TEST.BOOL(!text.IsActive());
            EZ_TEST.BOOL(!text.IsActiveAndInitialized());
            
            text.SetActive(true);
            EZ_TEST.BOOL(text.IsActive());
            EZ_TEST.BOOL(text.IsActiveAndInitialized());
            
            mesh.SetActive(false);
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
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestComponent") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

