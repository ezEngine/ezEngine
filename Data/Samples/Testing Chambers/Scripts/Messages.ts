import ez = require("./../TypeScript/ez")

export class MyTestMessage extends ez.Message {
    EZ_DECLARE_MESSAGE_TYPE;
}

export class MySecondMessage extends ez.Message {
    EZ_DECLARE_MESSAGE_TYPE;    
    text:string = "hello";
}