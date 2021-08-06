
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const ps = require('ps-node');

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

function findServerExe(suffix) {
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
        let isRunning = false; //await isServerRunning();
        if (!isRunning) {
            const exePath = findServerExe('build_vs/server.exe');
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

console.log(findServerExe('build_vs/server.exe'))

describe("Comms test suite", () => {
    beforeAll(async () => {
        console.log('Ready to start');
        await startServer();
    });

    afterAll(() => {
        if (serverProcess) {
            serverProcess.kill();
        }
    });

    test("Test1", (done) => {
        console.log('test goes here');

        const WebSocket = require('ws')
        const url = 'ws://localhost:8100'
        const connection = new WebSocket(url)

        connection.onopen = () => {
            for (let i = 0; i < 100; ++i)
                connection.send(`Message From Client ${i}`)
        }

        connection.onerror = (error) => {
            console.log(`WebSocket error: ${error}`)
        }

        let k = 0;
        connection.onmessage = (e) => {
            console.log(e.data)
            ++k;
            if (k == 100) {
                connection.close();
                done();
            }
        };
    });

});

