const cacheName = "appCache";
const staticAssets = [
  "./",
  "./img/icons-192.png",
  "./img/icons-512.png",
  "./index.html",
  "./manifest.json",
  "./style.css",
];

self.addEventListener("install", async () => {
  const cache = await caches.open(cacheName);
  await cache.addAll(staticAssets);
  return self.skipWaiting();
});

self.addEventListener("fetch", async (e) => {
  const req = e.request;
  const url = new URL(req.url);

  // console.log(e);

  if (url.origin === location.origin) e.respondWith(cacheFirst(req));
  else e.respondWith(networkAndCache(req));
});

self.addEventListener("activate", () => {
  self.client.claim();
});

async function cacheFirst(req) {
  const cache = await caches.open(cacheName);
  const cached = await cache.match(req);
  return cached || fetch(req);
}

async function networkAndCache(req) {
  const cache = await caches.open(cacheName);
  try {
    const fresh = await fetch(req);
    await cache.put(req, fresh.clone());
  } catch (e) {
    const cached = await cache.match(req);
    console.log(e);
    return cached;
  }
}
