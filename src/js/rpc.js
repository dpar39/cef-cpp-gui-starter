
const IS_NODE = typeof module !== 'undefined' && module.exports;
const protobuf = require('protobufjs')

let WebSocket = null;
class RpcService {
    constructor(url) {
        if (!url) {
            if (!IS_NODE) {
                url = window.location.origin.replace('http://', 'ws://');
                WebSocket = window.WebSocket;
            }
            else {
                url = 'ws://localhost:8100';
                WebSocket = require('ws');

            }
        }
        this.ws = new WebSocket(url)
        this.ws.binaryType = IS_NODE ? 'nodebuffer' : 'arraybuffer';
        this.pendingRequests = new Map();
        this.requestId = 0;
        this.protos = null;
        this.Message = null;

        this.ws.onmessage = async (e) => {
            await this.loadProtobuf();
            let msg = this.Message.decode(e.data);
            let obj = this.Message.toObject(msg);
            let msgId = obj.id;
            let cb = this.pendingRequests.get(msgId);
            if (cb.acceptCb)
                cb.acceptCb(obj.response);
            this.pendingRequests.delete(msgId);
        }
    }

    async close() {
        return new Promise((accept, reject) => {
            if (this.ws.readyState === WebSocket.CLOSED) {
                accept(true);
            }
            this.ws.onclose = (e) => {
                accept(true)
            };
            this.ws.close();
        }); 
    }

    loadProtobuf() {
        return new Promise((accept, reject) => {
            if (this.protos)
                return accept();
            protobuf.load('proto/messages.proto', (err, root) => {
                if (err) {
                    reject(err);
                } else {
                    this.protos = root;
                    this.Message = root.lookupType('comms.Message');
                    accept();
                }
            });
        });
    }

    async socketReady() {
        return new Promise((accept, reject) => {
            if (this.ws.readyState === WebSocket.OPEN) {
                accept(true);
            }
            this.ws.onopen = (e) => {
                console.log('Websocket ready');
                accept(true)
            };
        });
    }

    async makeRequest(request) {
        await this.socketReady();
        await this.loadProtobuf();
        let id = ++this.requestId;
        let msg = { id: id, request: request };
        let errMsg = this.Message.verify(msg);
        if (errMsg)
            throw Error(errMsg);
        let reqMsg = this.Message.create(msg);
        let buffer = this.Message.encode(reqMsg).finish();

        let cb = {};
        let pending = new Promise((accept, reject) => {
            cb.acceptCb = accept;
            cb.rejectCb = reject;
        });
        this.pendingRequests.set(id, cb)
        this.ws.send(buffer);
        return pending;
    }
}

if (IS_NODE) {
    module.exports = { RpcService }
}