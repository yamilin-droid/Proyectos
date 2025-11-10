// Proyect 6.js

// ---------------- Utilidades de generación ----------------

function makeArray(size, scenario) {
  size = Math.max(1, Math.min(size, 100000));
  if (scenario === "ordenado") {
    return Array.from({ length: size }, (_, i) => i);
  } else if (scenario === "medianamente") {
    // casi ordenado: 10% de swaps aleatorios
    const a = Array.from({ length: size }, (_, i) => i);
    const swaps = Math.max(1, Math.floor(size / 10));
    for (let s = 0; s < swaps; s++) {
      const i = Math.floor(Math.random() * size);
      const j = Math.floor(Math.random() * size);
      [a[i], a[j]] = [a[j], a[i]];
    }
    return a;
  } else if (scenario === "inversos") {
    return Array.from({ length: size }, (_, i) => size - 1 - i);
  }
  // fallback aleatorio (no usado por UI)
  return Array.from({ length: size }, () => Math.floor(Math.random() * (size * 10 + 1)));
}

function clone(a) { return a.slice(); }

// ---------------- Métodos de ordenamiento ----------------

function bubbleSort(arr) {
  const a = clone(arr);
  const n = a.length;
  for (let i = 0; i < n; i++) {
    let swapped = false;
    for (let j = 0; j < n - i - 1; j++) {
      if (a[j] > a[j + 1]) {
        [a[j], a[j + 1]] = [a[j + 1], a[j]];
        swapped = true;
      }
    }
    if (!swapped) break;
  }
  return a;
}

function insertionSort(arr) {
  const a = clone(arr);
  for (let i = 1; i < a.length; i++) {
    const key = a[i];
    let j = i - 1;
    while (j >= 0 && a[j] > key) {
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = key;
  }
  return a;
}

function selectionSort(arr) {
  const a = clone(arr);
  const n = a.length;
  for (let i = 0; i < n; i++) {
    let mi = i;
    for (let j = i + 1; j < n; j++) {
      if (a[j] < a[mi]) mi = j;
    }
    if (mi !== i) [a[i], a[mi]] = [a[mi], a[i]];
  }
  return a;
}

function quickSort(arr) {
  const a = clone(arr);
  if (a.length < 2) return a;

  function partition(lo, hi) {
    const pivot = a[hi];
    let i = lo;
    for (let j = lo; j < hi; j++) {
      if (a[j] <= pivot) {
        [a[i], a[j]] = [a[j], a[i]];
        i++;
      }
    }
    [a[i], a[hi]] = [a[hi], a[i]];
    return i;
  }

  const stack = [[0, a.length - 1]];
  while (stack.length) {
    let [lo, hi] = stack.pop();
    while (lo < hi) {
      const p = partition(lo, hi);
      const leftLen = p - 1 - lo;
      const rightLen = hi - (p + 1);
      if (leftLen > rightLen) {
        if (lo < p - 1) stack.push([lo, p - 1]);
        lo = p + 1;
      } else {
        if (p + 1 < hi) stack.push([p + 1, hi]);
        hi = p - 1;
      }
    }
  }
  return a;
}

function bucketSort(arr) {
  if (arr.length === 0) return [];
  const a = clone(arr);
  let mn = a[0], mx = a[0];
  for (const v of a) { if (v < mn) mn = v; if (v > mx) mx = v; }
  if (mn === mx) return a;

  const n = a.length;
  const k = Math.max(1, Math.floor(Math.sqrt(n)));
  const buckets = Array.from({ length: k }, () => []);
  const rng = mx - mn;

  for (const v of a) {
    const idx = Math.floor(((v - mn) * (k - 1)) / rng);
    buckets[idx].push(v);
  }

  const out = [];
  for (const b of buckets) {
    // insertion sort local
    for (let i = 1; i < b.length; i++) {
      const key = b[i];
      let j = i - 1;
      while (j >= 0 && b[j] > key) {
        b[j + 1] = b[j];
        j--;
      }
      b[j + 1] = key;
    }
    out.push(...b);
  }
  return out;
}

function radixSort(arr) {
  if (arr.length === 0) return [];
  const a = clone(arr);
  for (const v of a) { if (v < 0) throw new Error("RadixSort requiere números no negativos"); }
  let mx = a[0];
  for (const v of a) if (v > mx) mx = v;

  for (let exp = 1; Math.floor(mx / exp) > 0; exp *= 10) {
    const count = Array(10).fill(0);
    const out = Array(a.length);

    for (let i = 0; i < a.length; i++) {
      count[Math.floor(a[i] / exp) % 10]++;
    }
    for (let i = 1; i < 10; i++) count[i] += count[i - 1];

    for (let i = a.length - 1; i >= 0; i--) {
      const d = Math.floor(a[i] / exp) % 10;
      out[--count[d]] = a[i];
    }
    for (let i = 0; i < a.length; i++) a[i] = out[i];
  }
  return a;
}

function heapSort(arr) {
  const a = clone(arr);
  const n = a.length;
  function heapify(n, i) {
    let largest = i;
    const l = 2 * i + 1;
    const r = 2 * i + 2;
    if (l < n && a[l] > a[largest]) largest = l;
    if (r < n && a[r] > a[largest]) largest = r;
    if (largest !== i) {
      [a[i], a[largest]] = [a[largest], a[i]];
      heapify(n, largest);
    }
  }
  for (let i = Math.floor(n / 2) - 1; i >= 0; i--) heapify(n, i);
  for (let i = n - 1; i > 0; i--) {
    [a[0], a[i]] = [a[i], a[0]];
    heapify(i, 0);
  }
  return a;
}

function mergeSort(arr) {
  const a = clone(arr);
  function msort(lo, hi) {
    if (hi - lo <= 1) return a.slice(lo, hi);
    const mid = Math.floor((lo + hi) / 2);
    const left = msort(lo, mid);
    const right = msort(mid, hi);
    const out = [];
    let i = 0, j = 0;
    while (i < left.length && j < right.length) {
      if (left[i] <= right[j]) out.push(left[i++]);
      else out.push(right[j++]);
    }
    while (i < left.length) out.push(left[i++]);
    while (j < right.length) out.push(right[j++]);
    return out;
  }
  return msort(0, a.length);
}

function hashSort(arr) {
  if (arr.length === 0) return [];
  const counts = new Map();
  for (let i = 0; i < arr.length; i++) {
    const v = arr[i];
    counts.set(v, (counts.get(v) || 0) + 1);
  }
  const keys = Array.from(counts.keys());
  keys.sort((x, y) => x - y);
  const out = [];
  for (let i = 0; i < keys.length; i++) {
    const k = keys[i];
    let c = counts.get(k);
    while (c-- > 0) out.push(k);
  }
  return out;
}

const METHODS = {
  burbuja: { label: "Burbuja", run: bubbleSort },
  insercion: { label: "Inserción", run: insertionSort },
  seleccion: { label: "Selección", run: selectionSort },
  quicksort: { label: "Quicksort", run: quickSort },
  cubeta: { label: "Cubeta", run: bucketSort },
  radix: { label: "RadixSort", run: radixSort },
  heap: { label: "HeapSort", run: heapSort },
  merge: { label: "MergeSort", run: mergeSort },
  hash: { label: "Hashsort", run: hashSort },
};

const ALL_METHOD_KEYS = Object.keys(METHODS);

// ---------------- UI y lógica ----------------

const sizeEl = document.getElementById("size");
const scenarioEl = document.getElementById("scenario");
const methodEl = document.getElementById("method");
const runBtn = document.getElementById("run");
const nextBtn = document.getElementById("next");
const showChartBtn = document.getElementById("showChart");
const resultsBody = document.querySelector("#results tbody");
const chartCard = document.getElementById("chartCard");
const chartCanvas = document.getElementById("chart");
const summaryCard = document.getElementById("summaryCard");
const summaryList = document.getElementById("summaryList");

const DISPLAY_LIMIT = 200;

function getHeapUsed() {
  if (typeof performance !== "undefined" && performance.memory && typeof performance.memory.usedJSHeapSize === "number") {
    return performance.memory.usedJSHeapSize;
  }
  return null;
}
function estimateArrayBytes(a) {
  return (Array.isArray(a) ? a.length : 0) * 8;
}
function formatBytes(bytes) {
  if (bytes == null) return "-";
  const b = Math.max(0, bytes);
  if (b >= 1024 * 1024) return (b / (1024 * 1024)).toFixed(2) + " MB";
  if (b >= 1024) return (b / 1024).toFixed(2) + " KB";
  return b + " B";
}

let baseArray = [];
let queue = [];
let times = {}; let mems = {};
let finished = new Set();
let running = false;

function resetState() {
  queue = [];
  times = {}; mems = {};
  finished = new Set();
  running = false;
  resultsBody.innerHTML = "";
  nextBtn.disabled = true;
  showChartBtn.disabled = true;
  chartCard.style.display = "none";
  summaryCard.style.display = "none";
  const ctx = chartCanvas.getContext("2d");
  ctx.clearRect(0, 0, chartCanvas.width, chartCanvas.height);
}

function appendResultRow(methodKey, timeMs, memBytes, sorted) {
  const tr = document.createElement("tr");

  const tdM = document.createElement("td");
  tdM.textContent = METHODS[methodKey].label;

  function formatDuration(ms) {
    if (ms >= 1000) {
      return (ms / 1000).toFixed(2) + " s";
    } else if (ms >= 1) {
      return ms.toFixed(2) + " ms";
    } else {
      return Math.round(ms * 1000) + " microsegundos";
    }
  }

  const tdT = document.createElement("td");
  tdT.textContent = formatDuration(timeMs);

  const tdMem = document.createElement("td");
  tdMem.textContent = formatBytes(memBytes);

  const tdS = document.createElement("td");
  const pre = document.createElement("pre");
  const limit = 15;
  if (sorted.length > limit) {
    pre.textContent = sorted.slice(0, limit).join(", ") + ", ...";
  } else {
    pre.textContent = sorted.join(", ");
  }
  tdS.appendChild(pre);

  tr.appendChild(tdM);
  tr.appendChild(tdT);
  tr.appendChild(tdMem);
  tr.appendChild(tdS);
  resultsBody.appendChild(tr);
}

function planQueue(startMethod) {
  const idx = ALL_METHOD_KEYS.indexOf(startMethod);
  return [...ALL_METHOD_KEYS.slice(idx), ...ALL_METHOD_KEYS.slice(0, idx)];
}

function runTimed(methodKey, arr) {
  const sorter = METHODS[methodKey].run;
  const m0 = getHeapUsed();
  const t0 = performance.now();
  const sorted = sorter(arr);
  const t1 = performance.now();
  const m1 = getHeapUsed();
  const memBytes = (m0 != null && m1 != null) ? Math.max(0, m1 - m0) : estimateArrayBytes(sorted);
  return { sorted, ms: t1 - t0, memBytes };
}

async function runNext() {
  if (running || queue.length === 0) return;
  running = true;
  nextBtn.disabled = true;

  const m = queue.shift();
  const { sorted, ms, memBytes } = await new Promise((resolve) => {
    requestAnimationFrame(() => resolve(runTimed(m, baseArray)));
  });

  times[m] = ms;
  mems[m] = memBytes;
  finished.add(m);
  appendResultRow(m, ms, memBytes, sorted);

  running = false;
  if (queue.length > 0) {
    nextBtn.disabled = false;
  } else {
    showChartBtn.disabled = false;
    buildSummary(times);
  }
}

function buildSummary(timeMap) {
  const pairs = Object.entries(timeMap).sort((a, b) => a[1] - b[1]);
  summaryList.innerHTML = "";
  for (const [key, t] of pairs) {
    const li = document.createElement("li");
    const txt = t >= 1000 ? (t / 1000).toFixed(2) + " s" : (t >= 1 ? t.toFixed(2) + " ms" : Math.round(t * 1000) + " microsegundos");
    const ram = mems[key];
    const ramTxt = ram == null ? "-" : formatBytes(ram);
    li.textContent = `${METHODS[key].label}: ${txt} — RAM ${ramTxt}`;
    summaryList.appendChild(li);
  }
  summaryCard.style.display = "block";
}

function drawBarChart(dataMap) {
  chartCard.style.display = "block";
  const methods = ALL_METHOD_KEYS.filter(k => k in dataMap);
  // elegir unidad de escala según el máximo (en ms)
  const maxMs = Math.max(...methods.map(k => dataMap[k]), 1);
  let unitLabel = "ms";
  let factor = 1; // convierte ms -> unidad seleccionada
  if (maxMs >= 1000) { unitLabel = "s"; factor = 1 / 1000; }
  else if (maxMs < 1) { unitLabel = "microsegundos"; factor = 1000; }
  const values = methods.map(k => dataMap[k] * factor);

  const dpi = window.devicePixelRatio || 1;
  const W = chartCanvas.clientWidth || 800;
  const H = chartCanvas.clientHeight || 340;
  chartCanvas.width = W * dpi;
  chartCanvas.height = H * dpi;

  const ctx = chartCanvas.getContext("2d");
  ctx.setTransform(dpi, 0, 0, dpi, 0, 0);
  ctx.clearRect(0, 0, W, H);

  const m = { l: 60, r: 20, t: 20, b: 110 };
  const cw = W - m.l - m.r;
  const ch = H - m.t - m.b;
  const maxV = Math.max(...values, 1);
  const count = Math.max(methods.length, 1);
  const barW = (cw / count) * 0.6;
  const gap = (cw / count) * 0.4;

  // ejes
  ctx.strokeStyle = "#94a3b8";
  ctx.beginPath();
  ctx.moveTo(m.l, m.t);
  ctx.lineTo(m.l, m.t + ch);
  ctx.lineTo(m.l + cw, m.t + ch);
  ctx.stroke();

  // etiqueta eje Y
  ctx.save();
  ctx.fillStyle = "#475569";
  ctx.font = "12px system-ui, Arial";
  ctx.translate(16, m.t + ch / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.textAlign = "center";
  ctx.fillText(`Tiempo (${unitLabel})`, 0, 0);
  ctx.restore();

  // grid y ticks Y
  ctx.fillStyle = "#475569";
  ctx.font = "12px system-ui, Arial";
  const ticks = 5;
  for (let i = 0; i <= ticks; i++) {
    const val = (maxV * i) / ticks;
    const y = m.t + ch - (val / maxV) * ch;
    ctx.fillText(val.toFixed(0), 8, y + 4);
    ctx.strokeStyle = "#e2e8f0";
    ctx.beginPath();
    ctx.moveTo(m.l, y);
    ctx.lineTo(m.l + cw, y);
    ctx.stroke();
  }

  // barras
  methods.forEach((key, i) => {
    const v = dataMap[key] * factor;

    const h = (v / maxV) * ch;
    const x = m.l + i * (barW + gap) + gap * 0.5;
    const y = m.t + ch - h;

    ctx.fillStyle = "#0ea5e9";
    ctx.fillRect(x, y, barW, h);

    // etiqueta X debajo de la barra (sin rotación) con truncado para evitar solapamiento
    ctx.fillStyle = "#e6eef8"; // alto contraste
    ctx.textAlign = "center";
    ctx.textBaseline = "top";
    ctx.font = "12px system-ui, Arial";
    const label = METHODS[key].label;
    const maxW = Math.max(10, barW - 6);
    let shown = label;
    if (ctx.measureText(shown).width > maxW) {
      while (shown.length > 0 && ctx.measureText(shown + "…").width > maxW) {
        shown = shown.slice(0, -1);
      }
      shown = shown + "…";
    }
    ctx.fillText(shown, x + barW / 2, m.t + ch + 12);

    // valor arriba de la barra en microsegundos, alto contraste
    ctx.fillStyle = "#e6eef8";
    ctx.textAlign = "center";
    ctx.textBaseline = "alphabetic";
    ctx.font = "12px system-ui, Arial";
    const valueStr = unitLabel === "s" ? (v.toFixed(2) + " s") : (unitLabel === "ms" ? (v.toFixed(2) + " ms") : (Math.round(v) + " microsegundos"));
    ctx.fillText(valueStr, x + barW / 2, y - 6);
  });
}

// ---------------- Eventos ----------------

runBtn.addEventListener("click", async () => {
  resetState();
  const size = Number(sizeEl.value);
  const scenario = scenarioEl.value;
  baseArray = makeArray(size, scenario);

  queue = planQueue(methodEl.value);
  nextBtn.disabled = false;

  await runNext();
});

nextBtn.addEventListener("click", async () => {
  await runNext();
});

showChartBtn.addEventListener("click", () => {
  drawBarChart(times);
});