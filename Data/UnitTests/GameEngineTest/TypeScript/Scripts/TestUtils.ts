import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestUtils extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        // IsNumberEqual
        {
            EZ_TEST.BOOL(ez.Utils.IsNumberEqual(13, 14, 0.9) == false);
            EZ_TEST.BOOL(ez.Utils.IsNumberEqual(13, 14, 1.01) == true);
        }

        // IsNumberZero
        {
            EZ_TEST.BOOL(ez.Utils.IsNumberZero(0.1, 0.09) == false);
            EZ_TEST.BOOL(ez.Utils.IsNumberZero(0.1, 0.11) == true);

            EZ_TEST.BOOL(ez.Utils.IsNumberZero(-0.1, 0.09) == false);
            EZ_TEST.BOOL(ez.Utils.IsNumberZero(-0.1, 0.11) == true);
        }

        // StringToHash
        {
            EZ_TEST.BOOL(ez.Utils.StringToHash("a") != ez.Utils.StringToHash("b"))
        }

        // Clamp
        {
            EZ_TEST.INT(ez.Utils.Clamp(13, 8, 11), 11);
            EZ_TEST.INT(ez.Utils.Clamp(6, 8, 11), 8);
            EZ_TEST.INT(ez.Utils.Clamp(9, 8, 11), 9);
        }

        // Saturate
        {
            EZ_TEST.FLOAT(ez.Utils.Saturate(-0.7), 0, 0.001);
            EZ_TEST.FLOAT(ez.Utils.Saturate(0.3), 0.3, 0.001);
            EZ_TEST.FLOAT(ez.Utils.Saturate(1.3), 1.0, 0.001);
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestUtils") {

            this.ExecuteTests();
            msg.Message = "done";
        }
    }

}

