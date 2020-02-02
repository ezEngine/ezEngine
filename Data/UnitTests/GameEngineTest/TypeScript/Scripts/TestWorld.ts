import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestWorld extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    foundObjs: ez.GameObject[] = [];

    FoundObjectCallback = (go: ez.GameObject): boolean => {

        this.foundObjs.push(go);
        return true;
    }

    ExecuteTests(): boolean {

        // FindObjectsInSphere
        {
            this.foundObjs = [];
            ez.World.FindObjectsInSphere("Category1", new ez.Vec3(5, 0, 0), 3, this.FoundObjectCallback);
            EZ_TEST.INT(this.foundObjs.length, 2);

            this.foundObjs = [];
            ez.World.FindObjectsInSphere("Category2", new ez.Vec3(5, 0, 0), 3, this.FoundObjectCallback);
            EZ_TEST.INT(this.foundObjs.length, 1);
        }

        // FindObjectsInBox
        {
            this.foundObjs = [];
            ez.World.FindObjectsInBox("Category1", new ez.Vec3(-10, 0, -5), new ez.Vec3(0, 10, 5), this.FoundObjectCallback);
            EZ_TEST.INT(this.foundObjs.length, 3);

            this.foundObjs = [];
            ez.World.FindObjectsInBox("Category2", new ez.Vec3(-10, 0, -5), new ez.Vec3(0, 10, 5), this.FoundObjectCallback);
            EZ_TEST.INT(this.foundObjs.length, 2);
        }

        return false;
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestWorld") {

            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {
                msg.Message = "done";
            }
        
        }
    }

}

