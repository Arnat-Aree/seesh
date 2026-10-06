/* story.js (C) — กล่องเล่าเรื่องของขั้นที่เลือก */
function renderStory(step, index, total) {
  const $ = id => document.getElementById(id);
  if (!step) {
    $('step-no').textContent = '';
    $('step-title').textContent = 'เลือกคำสั่งทางซ้าย';
    $('step-text').textContent = '';
    $('step-tag').textContent = '';
    return;
  }
  $('step-no').textContent = `ขั้นที่ ${index + 1} / ${total}`;
  $('step-title').textContent = step.title;
  $('step-text').textContent = step.text;
  $('step-tag').textContent = step.syscall || '';
  $('step-tag').className = 'tag ' + (step.owner || 'A');   /* สีตามเจ้าของส่วนงาน */
}