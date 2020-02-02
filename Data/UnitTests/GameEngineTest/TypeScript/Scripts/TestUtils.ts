import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")
import prefab = require("./Prefab")

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

        // FindPrefabRootNode / FindPrefabRootScript / Exposed Script Parameters
        {
            let p1 = this.GetOwner().FindChildByName("Prefab1");
            let p2 = this.GetOwner().FindChildByName("Prefab2");

            EZ_TEST.BOOL(p1 != null);
            EZ_TEST.BOOL(p2 != null);

            {
                let p1r = ez.Utils.FindPrefabRootNode(p1);
                let p1s: prefab.Prefab = ez.Utils.FindPrefabRootScript(p1, "Prefab");

                EZ_TEST.BOOL(p1r != null);
                EZ_TEST.BOOL(p1r.GetName() == "root");

                EZ_TEST.BOOL(p1s != null);
                EZ_TEST.FLOAT(p1s.NumberVar, 11, 0.001);
                EZ_TEST.BOOL(p1s.BoolVar);
                EZ_TEST.BOOL(p1s.StringVar == "Hello");
                EZ_TEST.BOOL(p1s.Vec3Var.IsEqual(new ez.Vec3(1, 2, 3)));

                let c = new ez.Color();
                c.SetGammaByteRGBA(227, 106, 6, 255);
                EZ_TEST.BOOL(p1s.ColorVar.IsEqualRGBA(c));
            }

            {
                let p2r = ez.Utils.FindPrefabRootNode(p2);
                let p2s: prefab.Prefab = ez.Utils.FindPrefabRootScript(p2, "Prefab");

                EZ_TEST.BOOL(p2r != null);
                EZ_TEST.BOOL(p2r.GetName() == "root");

                EZ_TEST.BOOL(p2s != null);
                EZ_TEST.FLOAT(p2s.NumberVar, 2, 0.001);
                EZ_TEST.BOOL(p2s.BoolVar == false);
                EZ_TEST.BOOL(p2s.StringVar == "Bye");
                EZ_TEST.BOOL(p2s.Vec3Var.IsEqual(new ez.Vec3(4, 5, 6)));

                let c = new ez.Color();
                c.SetGammaByteRGBA(6, 164, 227, 255);
                EZ_TEST.BOOL(p2s.ColorVar.IsEqualRGBA(c));
            }            
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestUtils") {

            this.ExecuteTests();
            msg.Message = "done";
        }
    }

}

