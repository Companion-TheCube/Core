/* ===========================================================
   TheCube Control Panel — mock-enabled SPA
   =========================================================== */
(() => {
  const USE_MOCKS = true; // flip to false when wiring real endpoints

  /* ---------- tiny helpers ---------- */
  const $ = (sel, el=document) => el.querySelector(sel);
  const $$ = (sel, el=document) => [...el.querySelectorAll(sel)];
  const sleep = (ms) => new Promise(r => setTimeout(r, ms));

  const toasts = $("#toasts");
  const toast = (msg, dur=2200) => {
    const el = document.createElement("div");
    el.className = "toast";
    el.textContent = msg;
    toasts.appendChild(el);
    setTimeout(()=>{ el.remove(); }, dur);
  };

  const save = (k,v)=>localStorage.setItem(k, JSON.stringify(v));
  const load = (k,d)=>JSON.parse(localStorage.getItem(k) || JSON.stringify(d));

  /* ---------- theme ---------- */
  const themeToggle = $("#themeToggle");
  const savedTheme = load("theme", "dark");
  document.documentElement.setAttribute("data-theme", savedTheme);
  themeToggle.checked = savedTheme === "light";
  themeToggle.addEventListener("change", () => {
    const theme = themeToggle.checked ? "light" : "dark";
    document.documentElement.setAttribute("data-theme", theme);
    save("theme", theme);
  });

  /* ---------- mock backend ---------- */
  const mockDB = {
    online: true,
    modeCloud: load("modeCloud", false),
    ip: "192.168.1.42",
    host: "thecube.local",
    net: "Wi-Fi",
    tempC: 48.7,
    memUsed: 1320, memTotal: 3996,
    bootTs: Date.now() - 1000*60*37, // ~37 mins ago
    ssids: ["CubeNet","Workshop","VanLab","Dragonfly","CoffeeBar"],
    reminders: load("reminders", [
      { id: 1, ts: Date.now()+1000*60*45, text: "Stretch break" },
      { id: 2, ts: Date.now()+1000*60*120, text: "Hydrate 💧" },
    ]),
    messages: [],
    logs: [],
    personality: load("personality", {
      playfulness: 6, cheekiness: 5, seriousness: 5, empathy: 7, responsiveness: 7
    })
  };

  // generate rolling logs
  const logStream = () => {
    const samples = [
      "llm: on-device intent matched: hydration_reminder",
      "audio: beamformer init ok",
      "mesh: link quality 0.82 to node 9F3C",
      "ui: animation 'sparkle' queued",
      "system: idle temp " + mockDB.tempC.toFixed(1) + "°C",
      "scheduler: 2 tasks due in next hour",
      "radio: lora ping rssi -92dBm snr 8.1dB",
      "trigger: greeting played"
    ];
    const line = `[${new Date().toLocaleTimeString()}] ${samples[Math.floor(Math.random()*samples.length)]}`;
    mockDB.logs.push(line);
    if (mockDB.logs.length > 100) mockDB.logs.shift();
  };
  setInterval(logStream, 1500);

  // simple mock fetch router
  const realFetch = window.fetch.bind(window);
  window.fetch = async (url, opts={}) => {
    if (!USE_MOCKS) return realFetch(url, opts);
    const { method="GET" } = opts;

    // simulate network latency
    const lag = 180 + Math.random()*180;
    await sleep(lag);

    const respond = (json, ok=true) => new Response(JSON.stringify(json), {
      status: ok ? 200 : 500,
      headers: { "Content-Type":"application/json" }
    });

    if (url.endsWith("/api/status")) {
      return respond({
        online: mockDB.online,
        net: mockDB.net,
        ip: mockDB.ip,
        host: mockDB.host,
        uptime: Math.floor((Date.now()-mockDB.bootTs)/1000),
        tempC: mockDB.tempC + (Math.random()*0.6-0.3),
        memUsed: mockDB.memUsed + Math.floor(Math.random()*20-10),
        memTotal: mockDB.memTotal,
        modeCloud: mockDB.modeCloud
      });
    }
    if (url.endsWith("/api/logs")) {
      return respond({ lines: mockDB.logs.slice(-100) });
    }
    if (url.endsWith("/api/restart") && method==="POST") {
      mockDB.logs.push(`[${new Date().toLocaleTimeString()}] system: restarting services...`);
      return respond({ ok: true });
    }
    if (url.endsWith("/api/scanNetworks")) {
      const shuffled = [...mockDB.ssids].sort(()=>Math.random()-0.5);
      return respond(shuffled.slice(0, 4 + Math.floor(Math.random()*2)));
    }
    if (url.endsWith("/api/network") && method==="POST") {
      return respond({ ok:true });
    }
    if (url.endsWith("/api/reminders")) {
      return respond({ reminders: mockDB.reminders });
    }
    if (url.endsWith("/api/reminder") && method==="POST") {
      const body = JSON.parse(opts.body||"{}");
      if (body.op === "add") {
        const id = Math.max(0,...mockDB.reminders.map(r=>r.id||0))+1;
        mockDB.reminders.push({ id, ts: body.ts, text: body.text });
      } else if (body.op === "delete") {
        mockDB.reminders = mockDB.reminders.filter(r=>r.id!==body.id);
      } else if (body.op === "update") {
        const r = mockDB.reminders.find(x=>x.id===body.id);
        if (r) { r.text = body.text; r.ts = body.ts; }
      }
      save("reminders", mockDB.reminders);
      return respond({ ok:true, reminders: mockDB.reminders });
    }
    if (url.endsWith("/api/cube/send") && method==="POST") {
      const { target, message } = JSON.parse(opts.body||"{}");
      const entry = { time: Date.now(), target, message, status: "sent" };
      mockDB.messages.unshift(entry);
      setTimeout(()=>{ entry.status = "ack"; }, 700 + Math.random()*900);
      return respond({ ok:true });
    }
    if (url.endsWith("/api/personality") && method==="POST") {
      const { trait, value } = JSON.parse(opts.body||"{}");
      mockDB.personality[trait] = value;
      save("personality", mockDB.personality);
      return respond({ ok:true });
    }
    if (url.endsWith("/trigger") && method==="POST") {
      const { event } = JSON.parse(opts.body||"{}");
      mockDB.logs.push(`[${new Date().toLocaleTimeString()}] trigger: ${event}`);
      return respond({ ok:true });
    }
    if (url.endsWith("/api/mode") && method==="POST") {
      const { cloud } = JSON.parse(opts.body||"{}");
      mockDB.modeCloud = !!cloud; save("modeCloud", mockDB.modeCloud);
      return respond({ ok:true });
    }

    // fallback
    return respond({ ok:false, error:"mock: unknown endpoint" }, false);
  };

  /* ---------- dashboard ---------- */
  const onlinePill = $("#onlinePill");
  const networkInfo = $("#networkInfo");
  const personalityMode = $("#personalityMode");
  const modeText = $("#modeText");
  const clockEl = $("#clock");
  const uptimeEl = $("#uptime");
  const memEl = $("#mem");
  const tempEl = $("#temp");
  const ipEl = $("#ipAddr");
  const hostEl = $("#hostName");

  function fmtUptime(s) {
    const h = Math.floor(s/3600), m = Math.floor((s%3600)/60);
    return `${h}h ${m}m`;
  }
  function fmtMem(u,t) { return `${u} / ${t} MB`; }

  async function refreshStatus() {
    try {
      const r = await fetch("/api/status");
      const s = await r.json();
      onlinePill.textContent = s.online ? "Online" : "Offline";
      onlinePill.style.background = s.online ? "rgba(37,244,238,.20)" : "rgba(255,107,107,.18)";
      networkInfo.textContent = s.net;
      modeText.textContent = s.modeCloud ? "Cloud + Local" : "Local Only";
      uptimeEl.textContent = fmtUptime(s.uptime);
      tempEl.textContent = `${s.tempC.toFixed(1)}°C`;
      memEl.textContent = fmtMem(s.memUsed, s.memTotal);
      ipEl.textContent = s.ip;
      hostEl.textContent = s.host;
    } catch(e) {
      toast("Status refresh failed");
    }
  }
  setInterval(()=>{ clockEl.textContent = new Date().toLocaleTimeString(); }, 1000);
  refreshStatus(); setInterval(refreshStatus, 3000);

  /* ---------- logs & restart ---------- */
  const logsEl = $("#logs");
  async function pollLogs() {
    try {
      const r = await fetch("/api/logs"); const { lines } = await r.json();
      logsEl.textContent = lines.join("\n");
      logsEl.scrollTop = logsEl.scrollHeight;
    } catch {}
  }
  setInterval(pollLogs, 1500);
  $("#restartBtn").addEventListener("click", async () => {
    $("#restartBtn").disabled = true;
    toast("Restarting services…");
    await fetch("/api/restart", { method:"POST" });
    await sleep(1200);
    $("#restartBtn").disabled = false;
    toast("Services restarted");
  });

  // mode toggle
  const modeToggle = $("#modeToggle");
  modeToggle.checked = load("modeCloud", false);
  modeToggle.addEventListener("change", async () => {
    await fetch("/api/mode", { method:"POST", headers:{'Content-Type':'application/json'}, body: JSON.stringify({ cloud: modeToggle.checked })});
    refreshStatus();
  });

  /* ---------- personality ---------- */
  const cubeFace = $("#cubeFace");
  const traitSummary = $("#traitSummary");

  const setFace = (p) => {
    // derive expression from traits
    const { playfulness, cheekiness, seriousness, empathy, responsiveness } = p;
    const joy = (playfulness + empathy - seriousness/2) / 2.5; // rough “mood”
    const cheek = cheekiness;

    // eyes
    const left = $(".eye.left", cubeFace);
    const right = $(".eye.right", cubeFace);
    const mouth = $(".mouth", cubeFace);

    const blink = Math.random() < 0.05;
    left.style.height = right.style.height = blink ? "4px" : "16px";

    // eye tilt & mouth shape
    const tilt = Math.min(8, Math.max(-8, cheek-5));
    left.style.transform  = `rotate(${tilt}deg)`;
    right.style.transform = `rotate(${-tilt}deg)`;

    const smile = Math.min(18, Math.max(-10, Math.round(joy-5)*3));
    mouth.style.borderRadius = smile >= 0 ? "10px/14px" : "16px/8px";
    mouth.style.height = `${14 + Math.abs(smile)}px`;
    mouth.style.transform = `translateX(-50%) translateY(${smile>=0?0:4}px)`;

    // glow power
    cubeFace.style.boxShadow =
      `inset 0 0 0 1px rgba(255,255,255,.12),
       0 0 ${18 + Math.max(0, joy)*2}px rgba(121,169,255,.35)`;

    // label
    const emoji = joy>6 ? "😜" : joy>3 ? "🙂" : joy>1 ? "😐" : "🫤";
    traitSummary.textContent = `${emoji} ${joy>6?"Playful":joy>3?"Balanced":joy>1?"Calm":"Stoic"}`;
    personalityMode.textContent = traitSummary.textContent.split(" ").slice(1).join(" ");
  };

  const personality = {...mockDB.personality};
  const bindSliders = () => {
    $$("#personality .slider-row").forEach(row => {
      const trait = row.dataset.trait;
      const input = $("input[type=range]", row);
      const out = $("output", row);
      input.value = personality[trait];
      out.value = personality[trait];
      input.addEventListener("input", () => {
        const v = Number(input.value);
        personality[trait] = v;
        out.value = v;
        setFace(personality);
      });
      input.addEventListener("change", async () => {
        await fetch("/api/personality", {
          method: "POST", headers:{'Content-Type':'application/json'},
          body: JSON.stringify({ trait, value: Number(input.value) })
        });
        toast(`Saved ${trait} = ${input.value}`);
        save("personality", personality);
      });
    });
    setInterval(()=>setFace(personality), 1200); // occasional blink/animate
    setFace(personality);
  };
  bindSliders();

  /* ---------- reminders ---------- */
  const list = $("#reminderList");
  const form = $("#reminderForm");
  const timeInput = $("#remTime");
  const textInput = $("#remText");

  const parseWhen = (tStr, nlStr) => {
    // returns epoch ms
    const now = new Date();
    if (nlStr) {
      const s = nlStr.trim().toLowerCase();

      // in 10m / in 2h / in 1h30m
      let m = s.match(/^in\s*(\d+)\s*m(in(utes)?)?$/);
      if (m) return Date.now() + Number(m[1]) * 60 * 1000;

      m = s.match(/^in\s*(\d+)\s*h(ours?)?(?:\s*(\d+)\s*m(.*)?)?$/);
      if (m) {
        const h = Number(m[1]), mm = Number(m[3]||0);
        return Date.now() + (h*60+mm)*60*1000;
      }

      // tomorrow 7:30am
      m = s.match(/^tomorrow\s+(\d{1,2})(?::(\d{2}))?\s*(am|pm)?$/);
      if (m) {
        const d = new Date(now); d.setDate(d.getDate()+1);
        let hr = Number(m[1]); const min = Number(m[2]||0);
        const ap = m[3]; if (ap) { if (ap==="pm" && hr<12) hr+=12; if (ap==="am" && hr===12) hr=0; }
        d.setHours(hr, min, 0, 0); return d.getTime();
      }

      // today 19:00
      m = s.match(/^today\s+(\d{1,2})(?::(\d{2}))?$/);
      if (m) {
        const d = new Date(now);
        d.setHours(Number(m[1]), Number(m[2]||0), 0, 0);
        return d.getTime();
      }
    }
    // fallback to time input HH:MM
    if (tStr) {
      const [hh,mm] = tStr.split(":").map(Number);
      const d = new Date(now);
      d.setHours(hh, mm, 0, 0);
      if (d.getTime() < now.getTime()) d.setDate(d.getDate()+1); // schedule next day
      return d.getTime();
    }
    return Date.now() + 15*60*1000;
  };

  const renderReminders = (items) => {
    list.innerHTML = "";
    items.sort((a,b)=>a.ts-b.ts).forEach(r => {
      const li = document.createElement("li");
      const time = new Date(r.ts).toLocaleString();
      li.innerHTML = `<span><strong>${time}</strong> — ${r.text}</span>`;
      const btns = document.createElement("div");
      const edit = document.createElement("button"); edit.className="ghost"; edit.textContent="Edit";
      const del = document.createElement("button"); del.className="danger"; del.textContent="Delete";
      btns.append(edit, del); li.append(btns);

      edit.onclick = async () => {
        const newText = prompt("Update description:", r.text) ?? r.text;
        const newTime = prompt("Update time (HH:MM or 'in 10m'):", "");
        const ts = parseWhen(newTime, newTime);
        await fetch("/api/reminder", { method:"POST", headers:{'Content-Type':'application/json'}, body: JSON.stringify({ op:"update", id:r.id, ts, text:newText }) });
        drawReminders();
        toast("Reminder updated");
      };
      del.onclick = async () => {
        await fetch("/api/reminder", { method:"POST", headers:{'Content':'application/json'}, body: JSON.stringify({ op:"delete", id:r.id }) });
        drawReminders();
        toast("Reminder deleted");
      };
      list.append(li);
    });
  };

  async function drawReminders(){
    const r = await fetch("/api/reminders"); const { reminders } = await r.json();
    renderReminders(reminders);
  }
  form.addEventListener("submit", async (e)=> {
    e.preventDefault();
    const ts = parseWhen(timeInput.value, textInput.value);
    await fetch("/api/reminder", { method:"POST", headers:{'Content-Type':'application/json'}, body: JSON.stringify({ op:"add", ts, text: textInput.value }) });
    textInput.value = ""; timeInput.value = "";
    drawReminders();
    toast("Reminder added");
  });
  $("#exportRem").addEventListener("click", ()=>{
    const data = load("reminders", []);
    const blob = new Blob([JSON.stringify(data, null, 2)], {type:"application/json"});
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = "thecube-reminders.json";
    a.click();
  });
  drawReminders();

  /* ---------- triggers ---------- */
  $$("#triggers .button-grid button").forEach(btn=>{
    btn.addEventListener("click", async () => {
      await fetch("/trigger", { method:"POST", headers:{'Content-Type':'application/json'}, body: JSON.stringify({ event: btn.dataset.event }) });
      toast(`Triggered: ${btn.textContent}`);
    });
  });

  /* ---------- network ---------- */
  $("#scanBtn").addEventListener("click", async () => {
    const r = await fetch("/api/scanNetworks"); const ssids = await r.json();
    const sel = $("#ssidList");
    sel.innerHTML = ssids.map(s=>`<option value="${s}">${s}</option>`).join("");
    toast(`Found ${ssids.length} networks`);
  });
  $("#wifiSave").addEventListener("click", async () => {
    const payload = {
      ssid: $("#ssidList").value,
      password: $("#wifiPass").value,
      mode: document.querySelector('input[name="ipMode"]:checked').value,
      static: {
        ip: $("#staticIp")?.value || "",
        gw: $("#staticGw")?.value || "",
        dns: $("#staticDns")?.value || ""
      }
    };
    await fetch("/api/network", { method:"POST", headers:{'Content-Type':'application/json'}, body: JSON.stringify(payload) });
    toast("Network settings saved");
  });
  $$('input[name="ipMode"]').forEach(radio => {
    radio.addEventListener("change", () => {
      const staticOn = radio.value === "static" && radio.checked;
      $("#staticFields").classList.toggle("hidden", !staticOn);
    });
  });

  /* ---------- messaging ---------- */
  const msgHist = $("#messageHistory");
  $("#msgForm").addEventListener("submit", async (e)=>{
    e.preventDefault();
    const id = $("#targetId").value.trim();
    const msg = $("#msgText").value.trim();
    if (!id || !msg) return;
    await fetch("/api/cube/send", { method:"POST", headers:{'Content-Type':'application/json'}, body: JSON.stringify({ target: id, message: msg })});
    const li = document.createElement("li");
    li.textContent = `→ ${id}: ${msg}  (sent)`;
    msgHist.prepend(li);
    $("#msgText").value = "";
    toast("Message sent");
  });

})();
