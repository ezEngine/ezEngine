import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestDebug extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        
        ez.Debug.RegisterCVar_Boolean("test.bool", true, "bool");
        EZ_TEST.BOOL(ez.Debug.ReadCVar_Boolean("test.bool") == true);
        ez.Debug.WriteCVar_Boolean("test.bool", false);
        EZ_TEST.BOOL(ez.Debug.ReadCVar_Boolean("test.bool") == false);
        
        ez.Debug.RegisterCVar_Int("test.int", 12, "int");
        EZ_TEST.BOOL(ez.Debug.ReadCVar_Int("test.int") == 12);
        ez.Debug.WriteCVar_Int("test.int", -12);
        EZ_TEST.BOOL(ez.Debug.ReadCVar_Int("test.int") == -12);
        
        ez.Debug.RegisterCVar_Float("test.float", 19, "float");
        EZ_TEST.BOOL(ez.Debug.ReadCVar_Float("test.float") == 19);
        ez.Debug.WriteCVar_Float("test.float", -19);
        EZ_TEST.BOOL(ez.Debug.ReadCVar_Float("test.float") == -19);

        ez.Debug.RegisterCVar_String("test.string", "hello", "string");
        EZ_TEST.BOOL(ez.Debug.ReadCVar_String("test.string") == "hello");
        ez.Debug.WriteCVar_String("test.string", "world");
        EZ_TEST.BOOL(ez.Debug.ReadCVar_String("test.string") == "world");
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestDebug") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

