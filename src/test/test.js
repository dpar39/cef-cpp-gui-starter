
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const ps = require('ps-node');

const { RpcService } = require('../js/rpc');

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
            const exePath = resolveFilePath('build_release/server.exe');
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

console.log(resolveFilePath('build_release/server.exe'))


describe("Comms test suite", () => {
    let rpc = null;
    beforeAll(async () => {
        console.log('Ready to start');
        await startServer();
        rpc = new RpcService(null);
    });

    afterAll(async () => {
        if (serverProcess) {
            serverProcess.kill();
        }
        await rpc.close();
    });

    test("Test Echo Rpc sync", async () => {
        for (let i = 0; i < 2000; ++i) {
            let text = `This is echo ${i}`;
            let res = await rpc.makeRequest({ echo: { text: text } });
            expect(res.echo.text).toBe(text);
        }

    });
    test("Test Echo Rpc Async", async () => {
        let promises = [];
        let texts = [];
        let text  = '.'.repeat(1000);
        for (let i = 0; i < 10000; ++i) {
            promises.push(rpc.makeRequest({ echo: { text: text } }));
        }
        results = await Promise.all(promises);
        // for (let i = 0; i < 2000; ++i) {
        //     expect(results[i].echo.text).toBe(texts[i]);
        // }
    });

    test("Test2", (done) => {
        done();
    });

});
