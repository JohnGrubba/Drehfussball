#include <Arduino.h>

// ─────────────────────────────────────────────
// Web UI HTML / CSS / JS (Stored in Flash)
// ─────────────────────────────────────────────
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Foosball Tournament Manager</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    :root { --bg: #222; --card: #333; --primary: #4dabf7; --accent: #ff6b6b; --text: #fff; }
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: var(--bg); color: var(--text); margin: 0; padding: 10px; }
    h1, h2, h3 { text-align: center; margin-top: 10px; margin-bottom: 10px; }
    .nav { display: flex; gap: 10px; justify-content: center; margin-bottom: 20px; flex-wrap: wrap; }
    button { padding: 10px 15px; font-size: 1rem; border: none; border-radius: 5px; cursor: pointer; color: white; background: #555; }
    button.active, button:hover { background: var(--primary); }
    .card { background: var(--card); padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.5); max-width: 800px; margin: 0 auto; margin-bottom: 20px; }
    .hidden { display: none !important; }
    input, select { padding: 10px; margin: 5px 0; width: 100%; box-sizing: border-box; background: #444; border: 1px solid #666; color: white; border-radius: 4px; }
    
    /* Scoreboard */
    .scoreboard { display: flex; justify-content: space-around; margin: 20px 0; }
    .team { width: 45%; text-align: center; }
    .score { font-size: 4rem; margin: 10px 0; font-weight: bold; }
    .blue-bg { color: var(--primary); }
    .red-bg { color: var(--accent); }
    .btn-group { display: flex; justify-content: space-between; gap: 10px; }
    .btn-add { background-color: #2b8a3e; flex: 1; font-size: 1.5rem; }
    .btn-sub { background-color: #c92a2a; flex: 1; font-size: 1.5rem; }
    
    /* Bracket */
    .bracket { display: flex; justify-content: center; gap: 20px; overflow-x: auto; padding: 20px 0; }
    .round { display: flex; flex-direction: column; justify-content: space-around; min-width: 150px; }
    .match { background: #444; margin: 10px 0; border-radius: 5px; padding: 10px; text-align: center; cursor: pointer; border: 1px solid #666; }
    .match:hover { border-color: var(--primary); }
    .match.active { box-shadow: 0 0 10px var(--primary); }
    
    /* Tables */
    table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }
    th, td { padding: 10px; text-align: left; border-bottom: 1px solid #555; }
    
    .list-item { display: flex; justify-content: space-between; padding: 10px; background: #444; margin-bottom: 5px; border-radius: 5px;}
  </style>
</head>
<body>
  <h1>Tournament Manager</h1>
  <div class="nav">
    <button onclick="showView('scoreboard')" id="nav-scoreboard">Live Match</button>
    <button onclick="showView('tournaments')" id="nav-tournaments">Tournaments</button>
    <button onclick="showView('bracket')" id="nav-bracket" class="hidden">Bracket / Standings</button>
  </div>

  <!-- Scoreboard View -->
  <div id="view-scoreboard" class="card">
    <h2 id="current-match-title">Casual Match</h2>
    <div class="scoreboard">
      <div class="team">
        <h2 class="blue-bg" id="team1-name">Team Blue</h2>
        <div class="score" id="score1">0</div>
        <div class="btn-group">
          <button class="btn-sub" onclick="sendCommand('t1_sub')">-1</button>
          <button class="btn-add" onclick="sendCommand('t1_add')">+1</button>
        </div>
      </div>
      <div class="team">
        <h2 class="red-bg" id="team2-name">Team Red</h2>
        <div class="score" id="score2">0</div>
        <div class="btn-group">
          <button class="btn-sub" onclick="sendCommand('t2_sub')">-1</button>
          <button class="btn-add" onclick="sendCommand('t2_add')">+1</button>
        </div>
      </div>
    </div>
    <button style="width:100%; background:#f59f00; padding:15px; font-size:1.2rem;" onclick="sendCommand('reset')">RESET MATCH</button>
    <button id="btn-save-match" class="hidden" style="width:100%; background:var(--primary); padding:15px; font-size:1.2rem; margin-top:10px;" onclick="saveMatchData()">SAVE MATCH & RETURN TO BRACKET</button>
  </div>

  <!-- Tournaments View -->
  <div id="view-tournaments" class="card hidden">
    <h2>Your Tournaments</h2>
    <div id="tournament-list"></div>
    <button style="width:100%; margin-top:20px; background:var(--primary);" onclick="showView('create')">CREATE NEW TOURNAMENT</button>
  </div>

  <!-- Create Tournament View -->
  <div id="view-create" class="card hidden">
    <h2>Create Tournament</h2>
    <label>Tournament Name:</label>
    <input type="text" id="t-name" placeholder="e.g. Friday Night Cup">
    
    <label>Mode:</label>
    <select id="t-mode">
      <option value="elimination">Elimination (Knockout)</option>
      <option value="seeding">Seeding (Round Robin)</option>
      <option value="both">Seeding + Elimination</option>
    </select>
    
    <label>Teams (comma separated):</label>
    <input type="text" id="t-teams" placeholder="Alice/Bob, Charlie/Dave, Eve/Frank, Grace/Heidi">
    
    <button style="width:100%; margin-top:20px; background:var(--primary);" onclick="createTournament()">START TOURNAMENT</button>
  </div>

  <!-- Bracket/Standings View -->
  <div id="view-bracket" class="card hidden">
    <h2 id="b-title">Tournament</h2>
    <div id="seeding-section"></div>
    <div id="bracket-section" class="bracket"></div>
  </div>

  <script>
    // --- WebSocket ---
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    window.addEventListener('load', init);

    function initWebSocket() {
      websocket = new WebSocket(gateway);
      websocket.onmessage = onMessage;
      websocket.onclose = () => setTimeout(initWebSocket, 2000);
    }

    let currentScore = { team1: 0, team2: 0 };
    function onMessage(event) {
      let data = JSON.parse(event.data);
      currentScore = data;
      document.getElementById('score1').innerHTML = data.team1;
      document.getElementById('score2').innerHTML = data.team2;
    }
    function sendCommand(cmd) { if(websocket && websocket.readyState === WebSocket.OPEN) websocket.send(cmd); }

    // --- IndexedDB & State ---
    let db;
    let tournaments = [];
    let activeTournament = null;
    let activeMatchId = null;

    function init() {
      initWebSocket();
      let request = indexedDB.open("FoosballTournaments", 1);
      request.onupgradeneeded = (e) => {
        let database = e.target.result;
        if (!database.objectStoreNames.contains('tournaments')) {
          database.createObjectStore('tournaments', { keyPath: 'id' });
        }
      };
      request.onsuccess = (e) => {
        db = e.target.result;
        loadTournaments();
      };
    }

    function saveDB(tournament) {
      let tx = db.transaction('tournaments', 'readwrite');
      tx.objectStore('tournaments').put(tournament);
      tx.oncomplete = loadTournaments;
    }

    function loadTournaments() {
      let tx = db.transaction('tournaments', 'readonly');
      let store = tx.objectStore('tournaments');
      let request = store.getAll();
      request.onsuccess = () => {
        tournaments = request.result;
        renderTournaments();
      };
    }

    // --- Navigation ---
    function showView(viewId) {
      document.querySelectorAll('.card').forEach(el => el.classList.add('hidden'));
      document.getElementById('view-' + viewId).classList.remove('hidden');
      document.querySelectorAll('.nav button').forEach(el => el.classList.remove('active'));
      let navBtn = document.getElementById('nav-' + viewId);
      if(navBtn) navBtn.classList.add('active');
    }

    // --- UI Rendering ---
    function renderTournaments() {
      let html = tournaments.length ? '' : '<p>No tournaments yet.</p>';
      tournaments.forEach(t => {
        html += `<div class="list-item">
          <strong>${t.name}</strong>
          <div>
            <button onclick="openTournament(${t.id})">Open</button>
            <button style="background:var(--accent)" onclick="deleteTournament(${t.id})">Del</button>
          </div>
        </div>`;
      });
      document.getElementById('tournament-list').innerHTML = html;
    }

    function deleteTournament(id) {
      if(!confirm("Are you sure?")) return;
      let tx = db.transaction('tournaments', 'readwrite');
      tx.objectStore('tournaments').delete(id);
      tx.oncomplete = () => { if(activeTournament && activeTournament.id===id){ activeTournament=null; document.getElementById('nav-bracket').classList.add('hidden'); } loadTournaments(); };
    }

    function createTournament() {
      let name = document.getElementById('t-name').value || "Unnamed Tournament";
      let mode = document.getElementById('t-mode').value;
      let teamsRaw = document.getElementById('t-teams').value.split(',').map(s=>s.trim()).filter(s=>s);
      if(teamsRaw.length < 2) return alert("Need at least 2 teams");

      let t = {
        id: Date.now(),
        name, mode,
        teams: teamsRaw.map((v, i) => ({ id: i, name: v, points: 0, gd: 0 })),
        matches: [], phase: mode === 'both' ? 'seeding' : mode
      };
      
      // Auto-generate matches based on phase
      generateMatches(t);
      saveDB(t);
      openTournament(t.id);
    }

    function generateMatches(t) {
      t.matches = [];
      if(t.phase === 'seeding' || t.phase === 'both') {
        // Round robin
        let idCounter = 1;
        for(let i=0; i<t.teams.length; i++) {
          for(let j=i+1; j<t.teams.length; j++) {
            t.matches.push({ id: idCounter++, t1: t.teams[i].id, t2: t.teams[j].id, s1:0, s2:0, played:false, round:"seeding" });
          }
        }
      } else {
        // Elimination bracket generation logic (simplistic power of 2)
        let numT = t.teams.length;
        let rounds = Math.ceil(Math.log2(numT));
        let bracketSize = Math.pow(2, rounds);
        
        let idCounter = 100;
        let cRound = [];
        for(let i=0; i<bracketSize/2; i++) {
          cRound.push({ id: idCounter++, t1: (i*2 < numT ? t.teams[i*2].id : null), t2: (i*2+1 < numT ? t.teams[i*2+1].id : null), s1:0, s2:0, played:false, round: rounds });
        }
        t.matches.push(...cRound);
        // We'd build subsequent rounds recursively here in a full app. For brevity, we just do QF/SF simply.
        // Needs a real bracket generator for perfect layout.
      }
    }

    function openTournament(tId) {
      activeTournament = tournaments.find(t => t.id === tId);
      document.getElementById('nav-bracket').classList.remove('hidden');
      renderActiveTournament();
      showView('bracket');
    }

    function renderActiveTournament() {
      if(!activeTournament) return;
      document.getElementById('b-title').innerText = activeTournament.name;
      
      let seedingHtml = "";
      if(activeTournament.phase === 'seeding' || activeTournament.mode === 'both') {
        seedingHtml += "<h3>Standings</h3><table><tr><th>Team</th><th>Pts</th><th>GD</th></tr>";
        // Recalculate standings
        activeTournament.teams.forEach(te => {te.points=0; te.gd=0;});
        activeTournament.matches.filter(m=>m.played && m.round==="seeding").forEach(m => {
          let t1 = activeTournament.teams.find(x=>x.id===m.t1);
          let t2 = activeTournament.teams.find(x=>x.id===m.t2);
          t1.gd += (m.s1 - m.s2); t2.gd += (m.s2 - m.s1);
          if(m.s1 > m.s2) t1.points += 3;
          else if(m.s1 < m.s2) t2.points += 3;
          else { t1.points+=1; t2.points+=1; }
        });
        
        let sorted = [...activeTournament.teams].sort((a,b) => b.points - a.points || b.gd - a.gd);
        sorted.forEach(t => {
          seedingHtml += `<tr><td>${t.name}</td><td>${t.points}</td><td>${t.gd}</td></tr>`;
        });
        seedingHtml += "</table><h3>Seeding Matches</h3>";
        
        activeTournament.matches.filter(m=>m.round==="seeding").forEach(m => {
          let t1n = activeTournament.teams.find(x=>x.id===m.t1).name;
          let t2n = activeTournament.teams.find(x=>x.id===m.t2).name;
          let ms = m.played ? `${m.s1} - ${m.s2}` : 'vs';
          seedingHtml += `<div class="match" onclick="playMatch(${m.id})">${t1n} <b>${ms}</b> ${t2n} ${m.played?'(Done)':''}</div>`;
        });
      }
      document.getElementById('seeding-section').innerHTML = seedingHtml;
      
      // Needs complex elimination render for full brackets...
    }

    function playMatch(mId) {
      activeMatchId = mId;
      let m = activeTournament.matches.find(x=>x.id===mId);
      document.getElementById('team1-name').innerText = activeTournament.teams.find(x=>x.id===m.t1).name;
      document.getElementById('team2-name').innerText = activeTournament.teams.find(x=>x.id===m.t2).name;
      document.getElementById('current-match-title').innerText = activeTournament.name + " - Match";
      document.getElementById('btn-save-match').classList.remove('hidden');
      
      sendCommand('reset'); // Rest score on ESP32
      showView('scoreboard');
    }

    function saveMatchData() {
      if(!activeTournament || !activeMatchId) return;
      let m = activeTournament.matches.find(x=>x.id===activeMatchId);
      m.s1 = currentScore.team1;
      m.s2 = currentScore.team2;
      m.played = true;
      saveDB(activeTournament);
      document.getElementById('btn-save-match').classList.add('hidden');
      document.getElementById('current-match-title').innerText = "Casual Match";
      document.getElementById('team1-name').innerText = "Team Blue";
      document.getElementById('team2-name').innerText = "Team Red";
      activeMatchId = null;
      renderActiveTournament();
      showView('bracket');
    }
  </script>
</body>
</html>
)rawliteral";