# SeeSh: See-Through Shell for Visualizing System Calls

**SeeSh** เป็น shell ที่เขียนขึ้นเองด้วยภาษา C โดยใช้ POSIX system call จริง ใช้งานได้เหมือน shell ทั่วไป และทุกครั้งที่รันคำสั่ง SeeSh จะส่งข้อมูลว่าตัวเองเรียก system call อะไรบ้าง ไปแสดงบนหน้าเว็บที่อธิบายการทำงานทีละขั้นด้วยภาษาที่เข้าใจง่าย

รายวิชา Operating Systems and System Calls Programming · Mini Project

## สมาชิก

| ชื่อ | รหัสนักศึกษา |
|---|---|
| นายอาณัฐ อารีย์ | 673380432-5 |
| นายพัทธดนย์ คำนัน | 673380416-3 |
| นายณัฐภัทร ฉ่ำตะคุ | 673380583-4 |

## สิ่งที่ต้องติดตั้ง

| ระบบ | ติดตั้ง |
|---|---|
| macOS | Xcode Command Line Tools (`xcode-select --install`) |
| Linux / WSL | `sudo apt install -y build-essential git` |

ใช้ browser ใดก็ได้ และต้องมี `perl` สำหรับรันชุดทดสอบ (มีมากับ macOS และ Ubuntu อยู่แล้ว)

> **WSL:** clone โปรเจกต์ไว้ใน home ของ Linux (`/home/<ชื่อ>`) ด้วย VS Code ที่ต่อ WSL อยู่ อย่า clone ไว้ใน `/mnt/c/...` เพราะ Git ฝั่ง Windows จะแปลงการขึ้นบรรทัดของทุกไฟล์

## วิธีรัน

เปิด Terminal 2 หน้าต่าง

```bash
# หน้าต่างที่ 1: เว็บเซิร์ฟเวอร์
make viewer
```

เปิด browser ไปที่ **http://localhost:8080**

```bash
# หน้าต่างที่ 2: shell
make run
```

พิมพ์คำสั่งใน seesh แล้วดูหน้าเว็บ คำสั่งจะขึ้นทางซ้าย กด ⏭ เพื่อดูทีละขั้น หรือ ▶ เพื่อเล่นอัตโนมัติ

Shell ใช้งานได้ปกติแม้ไม่ได้เปิดหน้าเว็บ

### คำสั่งตัวอย่าง

```
pwd
echo Hello > /tmp/hello.txt
ls | grep c | wc -l
cat < /tmp/hello.txt | sort > /tmp/sorted.txt
sleep 3 &
jobs
history
abcxyz
```

## ความสามารถ

**รองรับ**
- รันโปรแกรมภายนอก และแจ้งเมื่อหาโปรแกรมไม่เจอหรือรันไม่ได้
- Built-in: `cd`, `pwd`, `jobs`, `history`, `help`, `exit`
- Redirect `<`, `>`, `>>` ทั้งกับโปรแกรมภายนอกและ built-in
- Pipe หลายขั้น และใช้ร่วมกับ redirect
- Background job (`&`) และเก็บกวาด process ที่จบแล้ว ไม่มี zombie
- Ctrl+C หยุดเฉพาะโปรแกรมที่รันอยู่ shell ไม่ปิด
- ข้อความในเครื่องหมายคำพูด เช่น `echo "Hello   OS"`

**ไม่รองรับ**
- Scripting (`if`, `for`), ตัวแปร (`$VAR`), wildcard (`*.c`)
- `&&`, `||`, `;`
- `fg`, `!!` และ background pipeline (`ls | wc &`)
- ปุ่มลูกศรแก้ไขคำสั่งและ auto-complete

## การทดสอบ

```bash
make test
```

ป้อนคำสั่งชุดเดียวกันเข้าทั้ง seesh และ bash แล้วเทียบผล (32 กรณี) แต่ละข้อรันในโฟลเดอร์ชั่วคราว และถ้าค้างเกิน 5 วินาทีนับว่าไม่ผ่าน

Ctrl+C ต้องทดสอบด้วยมือ: รัน `sleep 30` แล้วกด Ctrl+C ต้องขึ้น prompt ใหม่ และกด Ctrl+C ที่ prompt ต้องไม่ปิด shell

## โครงสร้างโปรเจกต์

```
seesh/
├── src/                 # ตัว shell
│   ├── main.c           # วนรับคำสั่ง + prompt
│   ├── parser.c/.h      # แยกคำสั่ง → command_t
│   ├── executor.c/.h    # fork, execvp, waitpid, built-in + redirect
│   ├── builtins.c/.h    # cd, pwd, jobs, history, help
│   ├── redirect.c/.h    # <, >, >> (open, dup2)
│   ├── pipeline.c/.h    # pipe หลายขั้น
│   ├── signals.c/.h     # Ctrl+C, SIGCHLD
│   ├── jobs.c/.h        # background job
│   ├── history.c/.h     # history
│   └── events.c/.h      # ส่ง event ออกทาง FIFO
├── viewer/
│   ├── server.c         # เว็บเซิร์ฟเวอร์: อ่าน FIFO แล้วส่งให้ browser (SSE)
│   └── web/             # หน้าเว็บ Step Mode
├── tests/               # ชุดทดสอบ (make test)
└── docs/
    ├── plan.md          # แผนและนิยาม "เสร็จ"
    └── design.md        # โครงสร้างคำสั่ง, ฟังก์ชัน และรูปแบบ event
```

รายละเอียดการออกแบบอยู่ใน [docs/design.md](docs/design.md)

