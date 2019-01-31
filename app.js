const Http = require('http');
const Cluster = require('cluster');
const OperatingSystem = require('os');

const coreCount = OperatingSystem.cpus().length;

const verteilers = [
  'https://verteiler1.mediathekview.de/',
  'https://verteiler2.mediathekview.de/',
  'https://verteiler4.mediathekview.de/',
  'https://verteiler6.mediathekview.de/',
  'https://verteiler.mediathekviewweb.de/'
];

/* if (Cluster.isMaster) {
  for (let i = 0; i < coreCount; i++) {
    Cluster.fork();
  }
} else {
  console.log(`Worker ${process.pid} started`);
} */

createServer();

function createServer() {
  Http.createServer((request, response) => {
    if (request.method == 'GET' || request.method == "POST") {
      const verteiler = getRandomVerteiler();

      response.writeHead(301, {
        'Location': verteiler
      });

      response.end();
    } else {
      response.writeHead(400, {});
      response.end();
    }
  }).listen(8999);
}

function getRandomVerteiler() {
  const index = Math.floor(Math.random() * verteilers.length);
  return verteilers[index];
}