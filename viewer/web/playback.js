/* playback.js (C) — ปุ่มก่อนหน้า / เล่นอัตโนมัติ / ถัดไป และ slider */
const Playback = {
  index: 0, total: 0, timer: null, onChange: null,

  init(onChange) {
    this.onChange = onChange;
    document.getElementById('btn-prev').onclick = () => { this.stop(); this.go(this.index - 1); };
    document.getElementById('btn-next').onclick = () => { this.stop(); this.go(this.index + 1); };
    document.getElementById('btn-play').onclick = () => (this.timer ? this.stop() : this.play());
    document.getElementById('slider').oninput = e => { this.stop(); this.go(+e.target.value); };
  },

  setTotal(total, keepIndex) {
    this.total = total;
    if (!keepIndex) this.index = 0;
    this.index = Math.min(this.index, Math.max(0, total - 1));
    const s = document.getElementById('slider');
    s.max = Math.max(0, total - 1);
    s.value = this.index;
  },

  go(i) {
    if (!this.total) return;
    this.index = Math.max(0, Math.min(this.total - 1, i));
    document.getElementById('slider').value = this.index;
    this.onChange(this.index);
  },

  play() {
    if (this.index >= this.total - 1) this.go(0);
    document.getElementById('btn-play').textContent = '⏸';
    this.timer = setInterval(() => {
      if (this.index >= this.total - 1) return this.stop();
      this.go(this.index + 1);
    }, 1800);
  },

  stop() {
    clearInterval(this.timer);
    this.timer = null;
    document.getElementById('btn-play').textContent = '▶';
  },
};