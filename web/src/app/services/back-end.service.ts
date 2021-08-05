import { Injectable } from '@angular/core';

import { webSocket, WebSocketSubject } from 'rxjs/webSocket';

enum DataTypes {
  JS_OBJECT=1,
  BYTE_ARRAY=2

}

@Injectable({
  providedIn: 'root',
})
export class BackEndService {
  myWebSocket: WebSocketSubject<any>;

  private _textEncoder = new TextEncoder();
  constructor() {
    this.myWebSocket = webSocket('ws://localhost:8000');

    

    this.myWebSocket.subscribe(
      (msg) => this.onMessage,
      (err) => this.onError,
      () => console.log('complete')
    );
  }

  onMessage(data: any) {


  }

  onError(err: any) {}

  call(method: string, parameters: Array<any>, cb: (a: any)=> void) {

    const payload = this.preparePayload(method, parameters)

    this.myWebSocket.next(payload);
  }
  preparePayload(method: string, parameters: Array<any>) {
    
   const me = this._textEncoder.encode(method)

   for (const parm of parameters)
   {
     if (typeof parm === 'string' )

   }
    
  }
}
