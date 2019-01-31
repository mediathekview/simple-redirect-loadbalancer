const Http = require('http');

const server = new Http.Server();

const verteilers = [
  'https://verteiler1.mediathekview.de/',
  'https://verteiler2.mediathekview.de/',
  'https://verteiler4.mediathekview.de/',
  'https://verteiler6.mediathekview.de/',
  'https://verteiler.mediathekviewweb.de/'
];

function getRandomVerteiler() {
  const index = Math.floor(Math.random() * verteilers.length);
  return verteilers[index];
}

server.on('request', (request, response) => {
  const verteiler = getRandomVerteiler();

  response.writeHead(301, {
    'Location': verteiler
  });

  response.end();
});

server.listen(8999);