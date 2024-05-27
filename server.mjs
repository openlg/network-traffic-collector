import { createServer } from 'node:http';

let counter = 0;
const server = createServer((req, res) => {
    counter++;
    console.log(`${req.method} ${req.url} ${req.protocol || 'HTTP'}/${req.httpVersion}`);
    Object.keys(req.headers).forEach(key => {
        console.log(`> ${key}: ${req.headers[key]}`)
    });
    let contentLength = req.headers['content-length'];
    contentLength = contentLength ? Number(contentLength) : 0;
    if (contentLength > 0) {
        if (contentLength > 1024*1024) {
            console.log(`Request body size is ${contentLength}`);
        } else {
            req.on('data', (data) => {
                if (data && data.length > 0) {
                    if (data.length < 1024 * 1024) {
                        console.log(data.toString('utf8'));
                    } else {
                        console.log(`Request body size is ${data.length}`);
                    }
                }
            });
        }
    }

    if (counter%3 === 0) {
        res.writeHead(500, { 'Content-Type': 'application/json' });
        res.end('{"code": "failed"}');
    } else {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end('{"code": "ok"}');
    }

});
// starts a simple http server locally on port 3000
server.listen(3000, '0.0.0.0', () => {
    console.log('Listening on 0.0.0.0:3000');
});
