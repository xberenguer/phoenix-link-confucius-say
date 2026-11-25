const PREFIX = 'phoenix-cpp-demo:' + self.registration.scope + ':';
const CACHE = PREFIX + "d27116451a4644bcc648";
const FILES = ["./", "./index.html", "./app.js", "./style.css", "./control-config.js", "./source-config.js", "./phoenix.js", "./phoenix.wasm", "./phoenix.data", "./PROVENANCE.json", "./BUNDLE_MANIFEST.json", "./THIRD_PARTY_LICENSES.txt"];
self.addEventListener('install', event => {
  event.waitUntil((async () => {
    try {
      const cache = await caches.open(CACHE);
      await cache.addAll(FILES.map(file => new Request(new URL(file, self.registration.scope), { cache: 'reload' })));
      await self.skipWaiting();
    } catch (error) {
      await caches.delete(CACHE);
      throw error;
    }
  })());
});
self.addEventListener('activate', event => {
  event.waitUntil((async () => {
    const old = (await caches.keys()).filter(key => key.startsWith(PREFIX) && key !== CACHE);
    await Promise.all(old.map(key => caches.delete(key)));
    await self.clients.claim();
    if (old.length) {
      const windows = await self.clients.matchAll({ type: 'window' });
      for (const client of windows) {
        if (client.url.startsWith(self.registration.scope)) {
          // Starting navigation without awaiting it lets activation finish;
          // navigation fetches are held until this event has settled.
          client.navigate(client.url).catch(() => {});
        }
      }
    }
  })());
});
self.addEventListener('fetch', event => {
  if (event.request.method === 'GET' && event.request.url.startsWith(self.registration.scope)) {
    event.respondWith((async () => {
      const cache = await caches.open(CACHE);
      return (await cache.match(event.request)) || fetch(event.request);
    })());
  }
});
