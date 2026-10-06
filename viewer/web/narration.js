/* narration.js (C) — แปลง event ของ 1 คำสั่ง เป็น "ฉาก" (กล่องที่ต้องวาด) และ "ขั้นตอน" (เนื้อเรื่อง) */

const PROGRAM_DESC = {
  ls: 'แสดงรายชื่อไฟล์', cat: 'อ่าน/แสดงไฟล์', grep: 'คัดบรรทัดที่ตรง', wc: 'นับบรรทัด/คำ',
  sort: 'เรียงบรรทัด', head: 'เอาเฉพาะต้นๆ', tail: 'เอาเฉพาะท้ายๆ', echo: 'พิมพ์ข้อความ',
  sleep: 'รอตามเวลา', yes: 'พิมพ์ y ไม่หยุด', uniq: 'ตัดบรรทัดซ้ำ', ps: 'แสดง process',
};
const BUILTIN_DESC = { cd: 'เปลี่ยนโฟลเดอร์', pwd: 'แสดงโฟลเดอร์ปัจจุบัน', help: 'แสดงวิธีใช้', jobs: 'แสดงงานเบื้องหลัง' };

const byType = (events, type) => events.filter(e => e.type === type);
const first  = (events, type) => events.find(e => e.type === type);

/* ฉาก: กล่องเรียงจากซ้ายไปขวา */
function buildScene(cmd) {
  const ev = cmd.events;
  const parse = first(ev, 'parse');
  const items = [];
  if (!parse) return { items };

  const opens = byType(ev, 'open');
  const inFile = opens.find(o => o.data.mode === 'read');
  const outFile = opens.find(o => o.data.mode !== 'read');
  const outArrow = () => ({ kind: 'arrow', id: 'aout', label: outFile.data.mode === 'append' ? '>>' : '>' });
  const outBox = () => ({ kind: 'file', id: 'fout', icon: '📄', name: outFile.data.file,
                          desc: outFile.data.mode === 'append' ? 'เขียนต่อท้าย' : 'เขียนลงไฟล์' });

  const builtin = first(ev, 'builtin');
  if (builtin) {
    items.push({ kind: 'box', id: 'shell', icon: '🐚', name: 'seesh', desc: BUILTIN_DESC[builtin.data.name] || 'คำสั่งในตัว' });
    if (outFile) items.push(outArrow(), outBox());
    return { items, parse, builtin, outFile };
  }

  const forks = byType(ev, 'fork');
  const pidOf = i => (forks.find(f => f.data.index === i) || {}).data?.child;

  if (parse.data.bg)
    items.push({ kind: 'box', id: 'shell', icon: '🐚', name: 'seesh', desc: 'รับคำสั่งต่อได้ทันที' });
  if (inFile)
    items.push({ kind: 'file', id: 'fin', icon: '📄', name: inFile.data.file, desc: 'ไฟล์ต้นทาง' },
               { kind: 'arrow', id: 'ain', label: '<' });

  parse.data.names.forEach((n, i) => {
    if (i > 0) items.push({ kind: 'arrow', id: 'p' + i, label: 'ท่อ ' + i });
    items.push({ kind: 'box', id: 'c' + i, icon: '⚙️', name: n, desc: PROGRAM_DESC[n] || 'โปรแกรม', pid: pidOf(i) });
  });

  if (outFile) items.push(outArrow(), outBox());

  return { items, parse, inFile, outFile };
}

/* ขั้นตอนของ built-in (ทำใน shell เอง ไม่มี process ลูก) */
function builtinSteps(cmd, scene, steps) {
  const ev = cmd.events;
  const name = scene.builtin.data.name;
  const all = scene.items.map(it => it.id);
  const out = scene.outFile;

  if (out) {
    steps.push({ title: 'เปิดไฟล์',
                 text: `เปิดไฟล์ ${out.data.file} เพื่อ${out.data.mode === 'append' ? 'เขียนต่อท้าย' : 'เขียนทับ'}`,
                 syscall: 'open()', owner: 'B', on: ['shell', 'fout'] });
    steps.push({ title: 'เปลี่ยนทางออกของ shell ชั่วคราว',
                 text: `Shell จำทางออกเดิม (หน้าจอ) ไว้ก่อน แล้วเปลี่ยนทางออกของตัวเองไปที่ไฟล์ ${out.data.file}`,
                 syscall: 'dup() + dup2()', owner: 'B', on: all });
  }

  const text = name === 'cd'  ? 'Shell เรียก chdir() เปลี่ยนโฟลเดอร์ของตัวเอง ต้องทำใน shell เพราะถ้าให้ process ลูกทำ ผลจะหายไปพร้อมกับลูก'
             : name === 'pwd' ? `Shell เรียก getcwd() ถามระบบปฏิบัติการว่าตอนนี้อยู่โฟลเดอร์ไหน แล้วพิมพ์ออก${out ? 'ไปที่ไฟล์' : 'หน้าจอ'}`
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

  /* 4. open */
  const opens = byType(ev, 'open');
  if (opens.length)
    steps.push({ title: 'เปิดไฟล์',
                 text: opens.map(o => `เปิดไฟล์ ${o.data.file} เพื่อ${o.data.mode === 'read' ? 'อ่านข้อมูล' : o.data.mode === 'append' ? 'เขียนต่อท้าย' : 'เขียนทับ'}`).join('\n'),
                 syscall: 'open()', owner: 'B', on: [...procs, ...pipes, ...files] });

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

  /* 6. exec */
  const execs = byType(ev, 'exec');
  if (execs.length)
    steps.push({ title: 'เริ่มทำงาน',
                 text: `ผู้ช่วยแต่ละคนเปลี่ยนตัวเองเป็นโปรแกรมจริง: ${execs.map(e => e.data.name).join(', ')}` + (links.length ? '\nข้อมูลเริ่มไหลจากซ้ายไปขวา' : ''),
                 syscall: 'execvp()', owner: 'A', on: all, flow: links });

  /* 7. foreground: รอ */
  if (!parse.data.bg) {
    const waits = byType(ev, 'wait');
    if (waits.length) {
      const bad = waits.filter(w => w.data.status !== 0).map(w =>
        w.data.status === 127 ? `⚠️ หาโปรแกรม ${nameOf[w.data.child]} ไม่เจอ (status 127)`
                              : `⚠️ ${nameOf[w.data.child]} จบด้วย status ${w.data.status}`);
      steps.push({ title: 'รอทุกคนทำงานเสร็จ',
                   text: `Shell รอจนผู้ช่วย${waits.length > 1 ? 'ทุกคน' : ''}ทำงานเสร็จ แล้วจึงพร้อมรับคำสั่งถัดไป` + (bad.length ? '\n' + bad.join('\n') : ''),
                   syscall: 'waitpid()', owner: 'A', on: all, flow: links });
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