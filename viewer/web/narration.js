/* narration.js (C) — แปลง event ของ 1 คำสั่ง เป็น "ฉาก" (กล่องที่ต้องวาด) และ "ขั้นตอน" (เนื้อเรื่อง) */

const PROGRAM_DESC = {
  ls: 'แสดงรายชื่อไฟล์', cat: 'อ่าน/แสดงไฟล์', grep: 'คัดบรรทัดที่ตรง', wc: 'นับบรรทัด/คำ',
  sort: 'เรียงบรรทัด', head: 'เอาเฉพาะต้นๆ', tail: 'เอาเฉพาะท้ายๆ', echo: 'พิมพ์ข้อความ',
  sleep: 'รอตามเวลา', yes: 'พิมพ์ y ไม่หยุด', uniq: 'ตัดบรรทัดซ้ำ', ps: 'แสดง process',
};
const BUILTIN_DESC = { cd: 'เปลี่ยนโฟลเดอร์', pwd: 'แสดงโฟลเดอร์ปัจจุบัน', help: 'แสดงวิธีใช้',
                       jobs: 'แสดงงานเบื้องหลัง', history: 'แสดงคำสั่งที่เคยพิมพ์' };

/* errno → เหตุผลภาษาไทย (เลขเหล่านี้ตรงกันทั้ง macOS และ Linux) */
const ERRNO_TH = { 2: 'ไม่มีไฟล์หรือโฟลเดอร์นี้', 13: 'ไม่มีสิทธิ์เข้าถึง',
                   20: 'มีบางส่วนของ path ที่ไม่ใช่โฟลเดอร์', 21: 'เป็นโฟลเดอร์ ไม่ใช่ไฟล์' };
const reason = d => ERRNO_TH[d.errno] || d.error;

const byType = (events, type) => events.filter(e => e.type === type);
const first  = (events, type) => events.find(e => e.type === type);
const modeText = m => m === 'read' ? 'อ่านข้อมูล' : m === 'append' ? 'เขียนต่อท้าย' : 'เขียนทับ';

/* ฉาก: กล่องเรียงจากซ้ายไปขวา */
function buildScene(cmd) {
  const ev = cmd.events;
  const parse = first(ev, 'parse');
  const items = [];
  if (!parse) return { items };

  /* ไฟล์ทั้งที่เปิดได้และเปิดไม่ได้ */
  const fileEvents = ev.filter(e => e.type === 'open' || e.type === 'open_fail');
  const inFile  = fileEvents.find(o => o.data.mode === 'read');
  const outFile = fileEvents.find(o => o.data.mode !== 'read');
  const fileBox = (f, id, okDesc) => ({ kind: 'file', id, icon: '📄', name: f.data.file,
                                        desc: f.type === 'open_fail' ? '❌ เปิดไม่ได้' : okDesc });
  const outArrow = () => ({ kind: 'arrow', id: 'aout', label: outFile.data.mode === 'append' ? '>>' : '>' });
  const outDesc = () => outFile.data.mode === 'append' ? 'เขียนต่อท้าย' : 'เขียนลงไฟล์';

  const builtin = first(ev, 'builtin');
  if (builtin) {
    items.push({ kind: 'box', id: 'shell', icon: '🐚', name: 'seesh', desc: BUILTIN_DESC[builtin.data.name] || 'คำสั่งในตัว' });
    if (outFile) items.push(outArrow(), fileBox(outFile, 'fout', outDesc()));
    return { items, parse, builtin, outFile };
  }

  const forks = byType(ev, 'fork');
  const pidOf = i => (forks.find(f => f.data.index === i) || {}).data?.child;

  if (parse.data.bg)
    items.push({ kind: 'box', id: 'shell', icon: '🐚', name: 'seesh', desc: 'รับคำสั่งต่อได้ทันที' });
  if (inFile)
    items.push(fileBox(inFile, 'fin', 'ไฟล์ต้นทาง'), { kind: 'arrow', id: 'ain', label: '<' });

  parse.data.names.forEach((n, i) => {
    if (i > 0) items.push({ kind: 'arrow', id: 'p' + i, label: 'ท่อ ' + i });
    items.push({ kind: 'box', id: 'c' + i, icon: '⚙️', name: n, desc: PROGRAM_DESC[n] || 'โปรแกรม', pid: pidOf(i) });
  });

  if (outFile) items.push(outArrow(), fileBox(outFile, 'fout', outDesc()));

  return { items, parse, inFile, outFile };
}

/* ขั้นตอนของ built-in (ทำใน shell เอง ไม่มี process ลูก) */
function builtinSteps(cmd, scene, steps) {
  const ev = cmd.events;
  const name = scene.builtin.data.name;
  const all = scene.items.map(it => it.id);
  const out = scene.outFile;

  /* เปิดไฟล์ไม่สำเร็จ: shell ไม่ทำคำสั่งเลย */
  const fail = first(ev, 'open_fail');
  if (fail) {
    steps.push({ title: 'เปิดไฟล์ไม่สำเร็จ',
                 text: `เปิดไฟล์ ${fail.data.file} ไม่ได้: ${reason(fail.data)}\nShell จึงไม่ทำคำสั่ง ${name} และทางออกของ shell ยังเป็นหน้าจอเหมือนเดิม`,
                 syscall: 'open()', owner: 'B', on: all });
    return steps;
  }

  if (out) {
    steps.push({ title: 'เปิดไฟล์',
                 text: `เปิดไฟล์ ${out.data.file} เพื่อ${modeText(out.data.mode)}`,
                 syscall: 'open()', owner: 'B', on: ['shell', 'fout'] });
    steps.push({ title: 'เปลี่ยนทางออกของ shell ชั่วคราว',
                 text: `Shell จำทางออกเดิม (หน้าจอ) ไว้ก่อน แล้วเปลี่ยนทางออกของตัวเองไปที่ไฟล์ ${out.data.file}`,
                 syscall: 'dup() + dup2()', owner: 'B', on: all });
  }

  const dest = out ? 'ไปที่ไฟล์' : 'หน้าจอ';
  const text = name === 'cd'      ? 'Shell เรียก chdir() เปลี่ยนโฟลเดอร์ของตัวเอง ต้องทำใน shell เพราะถ้าให้ process ลูกทำ ผลจะหายไปพร้อมกับลูก'
             : name === 'pwd'     ? `Shell เรียก getcwd() ถามระบบปฏิบัติการว่าตอนนี้อยู่โฟลเดอร์ไหน แล้วพิมพ์ออก${dest}`
             : name === 'history' ? `Shell เก็บทุกคำสั่งที่พิมพ์ไว้ในหน่วยความจำของตัวเอง (ล่าสุด 100 คำสั่ง) แล้วพิมพ์ออก${dest} จึงต้องเป็น built-in`
             : name === 'jobs'    ? `Shell อ่านรายการงานเบื้องหลังที่ตัวเองจดไว้ แล้วพิมพ์ออก${dest}`
             : `Shell ทำคำสั่งนี้เองโดยไม่ต้องสร้าง process ลูก${out ? ' ผลลัพธ์ไหลลงไฟล์' : ''}`;
  steps.push({ title: 'ทำคำสั่งเองใน shell', text, syscall: scene.builtin.syscall, owner: 'A',
               on: all, flow: out ? ['aout'] : [] });

  if (byType(ev, 'dup2').some(d => d.data.target === 'terminal'))
    steps.push({ title: 'คืนทางออกเดิม',
                 text: 'Shell เปลี่ยนทางออกกลับไปที่หน้าจอเหมือนเดิม เพราะ built-in ทำงานในตัว shell เอง ถ้าไม่คืน prompt และคำสั่งถัดๆ ไปทั้งหมดจะถูกเขียนลงไฟล์ด้วย',
                 syscall: 'dup2()', owner: 'B', on: ['shell'] });
  return steps;
}

/* ขั้นตอน: เรียงตามชนิด event ไม่ใช่ตามลำดับที่มาถึง */
function buildSteps(cmd, scene) {
  const ev = cmd.events, steps = [];
  const parse = scene.parse;
  if (!parse) return steps;

  const ids    = test => scene.items.filter(test).map(it => it.id);
  const shell  = ids(it => it.id === 'shell');
  const procs  = ids(it => /^c\d+$/.test(it.id));
  const pipes  = ids(it => /^p\d+$/.test(it.id));
  const files  = ids(it => it.kind === 'file');
  const links  = ids(it => it.kind === 'arrow');
  const all    = scene.items.map(it => it.id);
  const nameOf = {};
  byType(ev, 'fork').forEach(f => { nameOf[f.data.child] = f.data.name; });

  /* 1. อ่านคำสั่ง */
  const why = [];
  if (scene.builtin) {
    why.push(`${scene.builtin.data.name} เป็นคำสั่งในตัว shell จึงทำเองได้เลย ไม่ต้องสร้าง process ลูก`);
    if (scene.outFile) why.push(`เห็นเครื่องหมาย ${scene.outFile.data.mode === 'append' ? '>>' : '>'} จึงต้องส่งผลลัพธ์ลงไฟล์แทนหน้าจอ`);
  } else {
    const n = parse.data.ncmds;
    why.push(n > 1 ? `เห็นเครื่องหมาย | จึงรู้ว่ามี ${n} โปรแกรมที่ต้องทำงานต่อกันเป็นสายพาน`
                   : `เป็นโปรแกรมภายนอก 1 ตัว คือ ${parse.data.names[0]}`);
    if (scene.inFile)  why.push('เห็นเครื่องหมาย < จึงต้องอ่านข้อมูลจากไฟล์แทนคีย์บอร์ด');
    if (scene.outFile) why.push(`เห็นเครื่องหมาย ${scene.outFile.data.mode === 'append' ? '>>' : '>'} จึงต้องส่งผลลัพธ์ลงไฟล์แทนหน้าจอ`);
    if (parse.data.bg) why.push('เห็นเครื่องหมาย & ท้ายคำสั่ง จึงต้องทำงานนี้แบบเบื้องหลัง');
  }
  steps.push({ title: 'อ่านคำสั่ง', text: why.join('\n'), syscall: 'parse', owner: 'A', on: shell });

  if (scene.builtin) return builtinSteps(cmd, scene, steps);

  /* 2. pipe */
  if (byType(ev, 'pipe').length)
    steps.push({ title: `เตรียมท่อส่งข้อมูล ${pipes.length} เส้น`,
                 text: 'วางท่อไว้ก่อน ให้ผลลัพธ์ของโปรแกรมหนึ่งไหลไปเป็นข้อมูลของโปรแกรมถัดไปได้',
                 syscall: 'pipe()', owner: 'B', on: pipes });

  /* 3. fork */
  const forks = byType(ev, 'fork');
  if (forks.length)
    steps.push({ title: `สร้างผู้ช่วย ${forks.length} คน`,
                 text: `Shell แยกตัวเองออกเป็น process ลูก คนละตัวต่อหนึ่งโปรแกรม (PID ${forks.map(f => f.data.child).join(', ')})\nตัว shell เองยังอยู่ และเป็นแม่ของผู้ช่วยทุกคน`,
                 syscall: 'fork()', owner: 'A', on: [...shell, ...procs, ...pipes] });

  /* 4. open (สำเร็จ และไม่สำเร็จ) */
  const opens = byType(ev, 'open');
  const openFails = byType(ev, 'open_fail');
  if (opens.length || openFails.length) {
    const lines = [
      ...opens.map(o => `เปิดไฟล์ ${o.data.file} เพื่อ${modeText(o.data.mode)}`),
      ...openFails.map(f => `❌ เปิดไฟล์ ${f.data.file} ไม่ได้: ${reason(f.data)}\nผู้ช่วยคนนี้จึงจบการทำงานทันที โดยไม่ได้รันโปรแกรม`),
    ];
    steps.push({ title: openFails.length ? 'เปิดไฟล์ไม่สำเร็จ' : 'เปิดไฟล์',
                 text: lines.join('\n'), syscall: 'open()', owner: 'B',
                 on: [...procs, ...pipes, ...files] });
  }

  /* 5. dup2 */
  const dups = byType(ev, 'dup2');
  if (dups.length) {
    const target = t => t.startsWith('pipe#') ? 'ท่อ ' + t.slice(5) : 'ไฟล์ ' + t.replace(/^file:/, '');
    const lines = dups.map(d => {
      const who = nameOf[d.pid] || 'PID ' + d.pid;
      return d.data.fd === 'stdin' ? `• ${who}: ทางเข้า (stdin) ← ${target(d.data.target)}`
                                   : `• ${who}: ทางออก (stdout) → ${target(d.data.target)}`;
    });
    steps.push({ title: 'เปลี่ยนทางเข้า-ออกของข้อมูล',
                 text: 'ปกติโปรแกรมอ่านจากคีย์บอร์ดและเขียนลงหน้าจอ ผู้ช่วยจึงต่อทางใหม่ก่อนเริ่มทำงาน\n' + lines.join('\n'),
                 syscall: 'dup2()', owner: 'B', on: all.filter(id => id !== 'shell') });
  }

  /* 6. exec (สำเร็จ และไม่สำเร็จ) */
  const execs = byType(ev, 'exec');
  const execFails = byType(ev, 'exec_fail');
  if (execs.length) {
    const failedPid = new Set(execFails.map(f => f.pid));
    const ok = execs.filter(e => !failedPid.has(e.pid)).map(e => e.data.name);
    const lines = [];
    if (ok.length)
      lines.push(`ผู้ช่วยเปลี่ยนตัวเองเป็นโปรแกรมจริง: ${ok.join(', ')}` + (links.length ? '\nข้อมูลเริ่มไหลจากซ้ายไปขวา' : ''));
    execFails.forEach(f => lines.push(
      `❌ ${f.data.name}: ${f.data.errno === 2 ? 'หาโปรแกรมนี้ไม่เจอในทุกโฟลเดอร์ที่อยู่ใน PATH' : reason(f.data)}`));
    steps.push({ title: !ok.length ? 'เริ่มทำงานไม่สำเร็จ' : execFails.length ? 'เริ่มทำงาน (บางคนไม่สำเร็จ)' : 'เริ่มทำงาน',
                 text: lines.join('\n'), syscall: 'execvp()', owner: 'A',
                 on: all, flow: ok.length ? links : [] });
  }

  /* 7. foreground: รอ */
  if (!parse.data.bg) {
    const waits = byType(ev, 'wait');
    if (waits.length) {
      const openFailPid = new Set(openFails.map(f => f.pid));
      const bad = waits.filter(w => w.data.status !== 0).map(w => {
        const who = nameOf[w.data.child], s = w.data.status;
        if (openFailPid.has(w.data.child)) return `⚠️ ${who} ไม่ได้รัน เพราะเปิดไฟล์ไม่สำเร็จ (status ${s})`;
        if (s === 127) return `⚠️ หาโปรแกรม ${who} ไม่เจอ (status 127)`;
        if (s === 126) return `⚠️ เจอ ${who} แต่รันไม่ได้ (status 126)`;
        return `⚠️ ${who} จบด้วย status ${s}`;
      });
      steps.push({ title: 'รอทุกคนทำงานเสร็จ',
                   text: `Shell รอจนผู้ช่วย${waits.length > 1 ? 'ทุกคน' : ''}ทำงานเสร็จ แล้วจึงพร้อมรับคำสั่งถัดไป` + (bad.length ? '\n' + bad.join('\n') : ''),
                   syscall: 'waitpid()', owner: 'A', on: all, flow: execFails.length || openFails.length ? [] : links });
    }
    return steps;
  }

  /* 7. background: ไม่รอ → SIGCHLD → เก็บกวาด */
  const bg = first(ev, 'bg_start');
  if (bg)
    steps.push({ title: 'ไม่รอ กลับไปรับคำสั่งต่อทันที',
                 text: `บันทึกเป็นงานเบื้องหลังหมายเลข [${bg.data.job}] ต่างจากคำสั่งปกติตรงที่ shell ไม่หยุดรอ คุณพิมพ์คำสั่งอื่นต่อได้เลย`,
                 syscall: 'ไม่เรียก waitpid', owner: 'C', on: all });
  const sig = first(ev, 'signal');
  if (sig)
    steps.push({ title: 'OS ส่งสัญญาณมาบอก',
                 text: `เมื่อ ${nameOf[sig.data.child] || 'process ลูก'} ทำงานจบ OS ส่งสัญญาณ SIGCHLD แจ้ง shell ว่าลูกทำงานเสร็จแล้ว`,
                 syscall: 'SIGCHLD', owner: 'C', on: all });
  const done = first(ev, 'bg_done');
  if (done)
    steps.push({ title: 'เก็บกวาด แล้วแจ้งผู้ใช้',
                 text: `Shell เรียก waitpid แบบไม่รอ (WNOHANG) เก็บกวาด process ที่จบแล้ว กันไม่ให้กลายเป็น zombie แล้วแจ้งว่า [${done.data.job}] Done`,
                 syscall: 'waitpid(WNOHANG)', owner: 'C', on: all });
  return steps;
}