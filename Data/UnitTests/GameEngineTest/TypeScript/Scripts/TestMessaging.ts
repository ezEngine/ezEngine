import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

class MyMessage extends ez.Message {
    EZ_DECLARE_MESSAGE_TYPE;

    text: string = "hello";
}

class MyMessage2 extends ez.Message {
    EZ_DECLARE_MESSAGE_TYPE;

    value: number = 0;
}

export class TestMessaging extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
        ez.TypescriptComponent.RegisterMessageHandler(MyMessage, "OnMyMessage");
        ez.TypescriptComponent.RegisterMessageHandler(MyMessage2, "OnMyMessage2");
    }

    step: number = 0;
    msgCount: number = 0;

    ExecuteTests(): boolean {

        if (this.step == 0) {
            let m = new MyMessage();

            m.text = "hello 1";
            this.SendMessage(m, true);
            EZ_TEST.BOOL(m.text == "Got: hello 1");

            m.text = "hello 2";
            this.SendMessage(m, false);
            EZ_TEST.BOOL(m.text == "Got: hello 2");

            m.text = "hello 3";
            this.GetOwner().SendMessage(m, true);
            EZ_TEST.BOOL(m.text == "Got: hello 3");

            m.text = "hello 4";
            this.GetOwner().GetParent().SendMessageRecursive(m, true);
            EZ_TEST.BOOL(m.text == "Got: hello 4");

            return true;
        }

        if (this.step == 1) {
            let m = new MyMessage2;
            m.value = 1;

            this.PostMessage(m);

            EZ_TEST.INT(this.msgCount, 0);

            return true;
        }

        if (this.step == 2) {
            
            EZ_TEST.INT(this.msgCount, 1);
            
            let m = new MyMessage2;
            m.value = 1;
            
            this.GetOwner().PostMessage(m);
            
            EZ_TEST.INT(this.msgCount, 1);

            return true;
        }

        if (this.step == 3) {
            
            EZ_TEST.INT(this.msgCount, 2);
            
            let m = new MyMessage2;
            m.value = 1;
            
            this.GetOwner().GetParent().PostMessage(m);
            
            EZ_TEST.INT(this.msgCount, 2);

            return true;
        }

        if (this.step == 4) {
            
            EZ_TEST.INT(this.msgCount, 2);
            
            let m = new MyMessage2;
            m.value = 1;
            
            this.GetOwner().GetParent().PostMessageRecursive(m);
            
            EZ_TEST.INT(this.msgCount, 2);

            return true;
        }

        if (this.step == 5) {
            
            EZ_TEST.INT(this.msgCount, 3);

            return true;
        }

        if (this.step == 6) {
            
            EZ_TEST.INT(this.msgCount, 3);

            return true;
        }

        return false;
    }

    OnMyMessage(msg: MyMessage) {

        msg.text = "Got: " + msg.text;
    }

    OnMyMessage2(msg: MyMessage2) {
        this.msgCount += msg.value;
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestMessaging") {

            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {
                
                EZ_TEST.INT(this.msgCount, 3);

                msg.Message = "done";
            }

            this.step += 1;
        }
    }

}

