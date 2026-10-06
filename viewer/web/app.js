/* app.js (A) — เชื่อมต่อ server, จัดกลุ่ม event ตามคำสั่ง และสั่งวาดหน้าจอ */
const cmds = new Map();      /* เลขคำสั่ง → { id, raw, events } */
let selected = null;
let scene = { items: [] }, steps = [];
let pending = false, jump = false;

function renderList() {
  document.getElementById('empty').style.display = cmds.size ? 'none' : '';
  const list = document.getElementById('cmd-list');
  list.innerHTML = '';
  [...cmds.values()].reverse().forEach(c => {                  /* คำสั่งล่าสุดอยู่บนสุด */
    const owners = [...new Set(c.events.map(e => e.owner))].sort()
      .map(o => `<span class="owner ${o}">${o}</span>`).join('');
    const b = document.createElement('button');
    b.className = 'cmd-item' + (c.id === selected ? ' sel' : '');
    b.innerHTML = `<div class="raw">${esc(c.raw)}</div><div class="meta">#${c.id}${owners}</div>`;
    b.onclick = () => { Playback.stop(); selected = c.id; jump = true; schedule(); };
    list.appendChild(b);
  });
}

function show(i) {
  renderFlow(document.getElementById('flow'), scene, steps[i]);
  renderStory(steps[i], i, steps.length);
}

function render() {
  pending = false;
  renderList();
  const c = cmds.get(selected);
  if (!c) {
    scene = { items: [] }; steps = [];
    document.getElementById('cmd-raw').textContent = '—';
    show(0);
    return;
  }
  document.getElementById('cmd-raw').textContent = c.raw;
  scene = buildScene(c);
  steps = buildSteps(c, scene);
  Playback.setTotal(steps.length, !jump);   /* คำสั่งเดิมได้ event เพิ่ม: อยู่ขั้นเดิม */
  jump = false;
  show(Playback.index);
}

function schedule() {                        /* รวมการวาดหลายครั้งให้เหลือครั้งเดียวต่อเฟรม */
  if (!pending) { pending = true; requestAnimationFrame(render); }
}

function onEvent(ev) {
  if (ev.type === 'reset') { Playback.stop(); cmds.clear(); selected = null; schedule(); return; }
  if (!cmds.has(ev.cmd)) cmds.set(ev.cmd, { id: ev.cmd, raw: '', events: [] });
  const c = cmds.get(ev.cmd);
  c.events.push(ev);
  if (ev.type === 'cmd_start') {
    c.raw = ev.data.raw;
    if (!Playback.timer) { selected = ev.cmd; jump = true; }   /* เลือกคำสั่งล่าสุดอัตโนมัติ */
  }
  schedule();
}

Playback.init(show);

const conn = document.getElementById('conn');
const es = new EventSource('/events');
es.onopen = () => {
  cmds.clear();                              /* server จะส่ง event ทั้งหมดมาใหม่ */
  conn.textContent = 'เชื่อมต่อ shell แล้ว';
  conn.className = 'pill on';
};
es.onerror = () => { conn.textContent = 'หลุดการเชื่อมต่อ กำลังลองใหม่...'; conn.className = 'pill'; };
es.onmessage = e => {
  try { onEvent(JSON.parse(e.data)); } catch (err) { console.error(err, e.data); }
};