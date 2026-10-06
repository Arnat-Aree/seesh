/* flow.js (B) — วาดกล่อง process ท่อ และไฟล์ ตามขั้นที่เลือก */
function esc(s) {
  return String(s).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
}

function renderFlow(el, scene, step) {
  const on = new Set(step ? step.on : []);
  const flowing = new Set(step && step.flow ? step.flow : []);
  el.innerHTML = '';

  scene.items.forEach(it => {
    const d = document.createElement('div');
    if (it.kind === 'arrow') {
      d.className = 'arrow' + (on.has(it.id) ? ' on' : '') + (flowing.has(it.id) ? ' flowing' : '');
      d.innerHTML = `<div class="shaft">→</div><div>${esc(it.label || '')}</div>`;
    } else {
      d.className = 'box' + (it.kind === 'file' ? ' file' : '') + (on.has(it.id) ? ' on' : '');
      d.innerHTML = `<div class="icon">${it.icon}</div>
                     <div class="name">${esc(it.name)}</div>
                     <div class="desc">${esc(it.desc)}</div>` +
                    (it.pid ? `<div class="pid">PID ${it.pid}</div>` : '');
    }
    el.appendChild(d);
  });
}