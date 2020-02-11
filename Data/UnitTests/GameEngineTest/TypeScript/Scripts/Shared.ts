import ez = require("TypeScript/ez")

export class MyMessage extends ez.Message {
    EZ_DECLARE_MESSAGE_TYPE;

    text: string = "hello";
}

export class MyMessage2 extends ez.Message {
    EZ_DECLARE_MESSAGE_TYPE;

    value: number = 0;
}

