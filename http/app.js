document.addEventListener('DOMContentLoaded', ()=> {
  // Theme toggle
  const themeBtn = document.getElementById('themeToggle');
  themeBtn.onclick = () => {
    const t = document.documentElement.getAttribute('data-theme') === 'dark' ? '' : 'dark';
    document.documentElement.setAttribute('data-theme', t);
    themeBtn.textContent = t ? '☀️' : '🌙';
  };

  // Dashboard: clock and heartbeat status polling
  const statusText = document.getElementById('statusText');
  const networkInfo = document.getElementById('networkInfo');
  const personalityMode = document.getElementById('personalityMode');
  function updateDashboard() {
    // mock fetch
    statusText.textContent = 'Online';
    networkInfo.textContent = 'WiFi';
    personalityMode.textContent = 'Balanced';
  }
  setInterval(updateDashboard, 5000);

  // Clock + mood
  const clockEl = document.getElementById('clock'),
        moodEl = document.getElementById('moodIndicator');
  function tick() {
    const now = new Date();
    clockEl.textContent = now.toLocaleTimeString();
  }
  setInterval(tick, 1000); tick();

  // Personality sliders
  const preview = document.getElementById('personalityPreview');
  document.querySelectorAll('#personality .slider-row').forEach(row=>{
    const trait = row.dataset.trait;
    const input = row.querySelector('input');
    input.oninput = ()=>{
      // update preview emoji based on playfulness only (example)
      if (trait==='playfulness') {
        const v = input.value;
        preview.textContent = v>7?'😜':v<4?'😐':'🙂';
      }
      // TODO: POST to /api/personality {trait, value}
    };
  });

  // Reminders
  const remList = document.getElementById('reminderList'),
        remForm = document.getElementById('reminderForm'),
        remTime = document.getElementById('reminderTime'),
        remText = document.getElementById('reminderText'),
        reminders = JSON.parse(localStorage.getItem('reminders')||'[]');
  function renderReminders(){
    remList.innerHTML = '';
    reminders.forEach((r,i)=>{
      const li = document.createElement('li');
      li.textContent = `${r.time} – ${r.text}`;
      const del = document.createElement('button');
      del.textContent = '✖'; del.onclick=()=>{
        reminders.splice(i,1);
        localStorage.setItem('reminders',JSON.stringify(reminders));
        renderReminders();
      };
      li.append(del);
      remList.append(li);
    });
  }
  remForm.onsubmit = e=>{
    e.preventDefault();
    reminders.push({time: remTime.value, text: remText.value});
    localStorage.setItem('reminders',JSON.stringify(reminders));
    remTime.value=''; remText.value='';
    renderReminders();
  };
  renderReminders();

  // Trigger events
  document.querySelectorAll('#triggers button').forEach(btn=>{
    btn.onclick = ()=>{
      fetch('/trigger', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body: JSON.stringify({event:btn.dataset.event})
      });
    };
  });

  // Dev tools: logs & restart
  const logsEl = document.getElementById('logs'),
        restartBtn = document.getElementById('restartBtn'),
        systemEl = document.getElementById('systemInfo');
  function fetchLogs(){
    // mock
    logsEl.textContent = Array(10).fill().map((_,i)=>`Log ${i}`).join('\n');
  }
  restartBtn.onclick = ()=> fetch('/api/restart',{method:'POST'});
  setInterval(fetchLogs, 5000);

  function fetchSystem(){
    // mock
    systemEl.textContent = `Uptime: ${Math.floor(performance.now()/1000)}s`;
  }
  setInterval(fetchSystem, 5000);

  // Network
  document.getElementById('scanBtn').onclick = ()=>{
    fetch('/api/scanNetworks').then(r=>r.json()).then(ssids=>{
      const sel = document.getElementById('ssidList');
      sel.innerHTML = ssids.map(s=>`<option>${s}</option>`).join('');
    });
  };
  document.getElementById('wifiSave').onclick = ()=>{
    fetch('/api/network', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({
        ssid: document.getElementById('ssidList').value,
        password: document.getElementById('wifiPass').value,
        mode: document.querySelector('input[name="ipMode"]:checked').value
      })
    });
  };

  // Messaging
  const msgForm = document.getElementById('msgForm'),
        msgHist = document.getElementById('messageHistory');
  msgForm.onsubmit = e=>{
    e.preventDefault();
    const id = document.getElementById('targetId').value,
          txt = document.getElementById('msgText').value;
    fetch('/api/cube/send',{method:'POST',headers:{'Content-Type':'application/json'},
      body: JSON.stringify({target:id,message:txt})
    }).then(()=>{
      const li = document.createElement('li');
      li.textContent = `To ${id}: ${txt}`;
      msgHist.prepend(li);
    });
  };
});
