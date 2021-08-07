
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const ps = require('ps-node');
const protobuf = require('protobufjs')

let serverProcess = null;
// function isServerRunning() {
//     return new Promise((accept, reject) => {
//         ps.lookup({
//             command: /server\.exe/,
//         }, (err, resultList) => {
//             if (err) {
//                 reject(err);
//             }
//             for (let p of resultList) {
//                 if (p.command.endsWith('/server.exe') || p.command.endsWith('/server')) {
//                     accept(true);
//                     return;
//                 }
//             }
//             accept(false);
//         });
//     });
// }

function resolveFilePath(suffix) {
    let dir = __dirname;
    while (true) {
        let exe_path = dir + '/' + suffix;
        if (fs.existsSync(exe_path)) {
            return exe_path;
        }
        let dirParent = path.dirname(dir)
        if (!dirParent || dirParent === dir) {
            throw new Error(`Can't find ${suffix} relative to ${__dirname}`);
        }
        dir = dirParent;
    }
}

function startServer() {
    return new Promise(async (accept, reject) => {
        let isRunning = true; //await isServerRunning();
        if (!isRunning) {
            const exePath = resolveFilePath('build_vs/server.exe');
            console.log(`Spawning ${exePath}`)
            serverProcess = spawn(exePath);
            let accepted = false;
            serverProcess.stdout.on('data', (data) => {
                console.log(data.toString());
                if (!accepted) {
                    accept();
                    accepted = true;
                }
            });
        } else {
            accept();
        }
    });
}

console.log(resolveFilePath('build_vs/server.exe'))

function loadProtobuf() {
    return new Promise((accept, reject) => {
        protobuf.load('proto/messages.proto', (err, root) => {
            if (err) {
                reject(err);
            } else {
                accept(root);
            }
        });
    });
}

describe("Comms test suite", () => {
    let pb = null;
    beforeAll(async () => {
        console.log('Ready to start');
        await startServer();
        pb = await loadProtobuf();
    });

    afterAll(() => {
        if (serverProcess) {
            serverProcess.kill();
        }
    });

    test("Test1", (done) => {
        console.log('test goes here');

        let Request = pb.lookupType('comms.Request');
        let Response = pb.lookupType('comms.Response');

        const WebSocket = require('ws')
        const url = 'ws://localhost:8100'
        const connection = new WebSocket(url)
        connection.binaryType = (typeof window === 'undefined') ? 'nodebuffer' : 'arraybuffer';

        connection.onopen = () => {
            // for (let i = 0; i < 100; ++i)
            //     connection.send(`Message From Client ${i}`)


            let req = { id: 123, listDir: { directory: '.' } }

            var errMsg = Request.verify(req);
            if (errMsg)
                throw Error(errMsg)
            let reqMsg = Request.create(req);
            console.log(reqMsg)
            let buffer = Request.encode(reqMsg).finish();
            connection.send(buffer)

        }

        connection.onerror = (error) => {
            console.log(`WebSocket error: ${error.message}`)
        }

        let k = 0;
        connection.onmessage = (e) => {

            let res = Response.decode(e.data);
            console.log(JSON.stringify(res));
            connection.close();
        };

        connection.onclose = () =>{
            done();
        };
    });

});
