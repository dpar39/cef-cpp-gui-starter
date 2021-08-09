
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


describe("Comms test suite", () => {
    let rpc = null;
    beforeAll(async () => {
        console.log('Ready to start');
        await startServer();
        rpc = new RpcService(null);
    });

    afterAll(() => {
        if (serverProcess) {
            serverProcess.kill();
        }
    });

    test("Test1", async () => {
        let res = await rpc.makeRequest({ listDir: { directory: '.' } });
        console.log(JSON.stringify(res));
    });

    test("Test2", (done) => {
        done();
    });

});
