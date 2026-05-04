#include <Arduino.h>

// ─────────────────────────────────────────────
// Web UI HTML / CSS / JS (Stored in Flash)
// ─────────────────────────────────────────────
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Foosball Tournament Manager</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    :root { 
      --bg: #121212; --card: #1e1e1e; --primary: #339af0; --primary-hover: #228be6;
      --accent: #ff6b6b; --accent-hover: #fa5252; --text: #e9ecef; --text-muted: #adb5bd;
      --border: #343a40; --live-glow: rgba(51, 154, 240, 0.6);
    }
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: var(--bg); color: var(--text); margin: 0; padding: 20px; }
    h1, h2, h3 { text-align: center; margin-top: 10px; margin-bottom: 20px; font-weight: 600; }
    h1 { font-size: 2.5rem; letter-spacing: 1px; color: var(--primary); }
    
    .nav { display: flex; gap: 15px; justify-content: center; margin-bottom: 30px; flex-wrap: wrap; }
    button { padding: 12px 20px; font-size: 1rem; border: none; border-radius: 8px; cursor: pointer; color: white; background: #495057; transition: all 0.2s; font-weight: bold; }
    button.active, button:hover { background: var(--primary); transform: translateY(-2px); box-shadow: 0 4px 10px rgba(0,0,0,0.3); }
    
    .card { background: var(--card); padding: 30px; border-radius: 12px; box-shadow: 0 8px 16px rgba(0,0,0,0.4); max-width: 1200px; margin: 0 auto; margin-bottom: 30px; border: 1px solid var(--border); }
    .hidden { display: none !important; }
    input, select { padding: 12px; margin: 10px 0 20px; width: 100%; box-sizing: border-box; background: #2b2b2b; border: 1px solid var(--border); color: white; border-radius: 6px; font-size: 1rem; }
    input:focus, select:focus { outline: none; border-color: var(--primary); box-shadow: 0 0 0 2px rgba(51,154,240,0.25); }
    label { font-weight: 600; color: var(--text-muted); }

    /* Scoreboard Live Match */
    .scoreboard { display: flex; justify-content: space-around; margin: 30px 0; align-items: center; }
    .team { width: 40%; text-align: center; background: #252525; padding: 20px; border-radius: 10px; border: 1px solid var(--border); }
    .score { font-size: 6rem; margin: 10px 0; font-weight: bold; text-shadow: 0 4px 8px rgba(0,0,0,0.5); }
    .blue-bg { color: var(--primary); font-size: 2rem; }
    .red-bg { color: var(--accent); font-size: 2rem; }
    .btn-group { display: flex; justify-content: space-between; gap: 15px; margin-top: 20px; }
    .btn-add { background-color: #2b8a3e; flex: 1; font-size: 2rem; border-radius: 8px; }
    .btn-sub { background-color: #c92a2a; flex: 1; font-size: 2rem; border-radius: 8px; }
    .btn-add:hover { background-color: #38d9a9; }
    .btn-sub:hover { background-color: #ff8787; }

    /* Full Bracket View */
    .bracket-container { overflow-x: auto; padding: 20px 0; display: flex; justify-content: center; width: 100%; }
    .bracket { display: flex; gap: 40px; }
    .round { display: flex; flex-direction: column; justify-content: space-around; min-width: 200px; gap: 20px; position: relative; }
    
    .match { background: #2b2b2b; border-radius: 8px; overflow: hidden; cursor: pointer; border: 1px solid var(--border); transition: all 0.2s; position: relative; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
    .match:hover { border-color: var(--primary); transform: scale(1.02); }
    
    /* Live Match Animation */
    @keyframes pulse-border {
      0% { box-shadow: 0 0 0 0 var(--live-glow); border-color: var(--primary); }
      70% { box-shadow: 0 0 0 15px rgba(51, 154, 240, 0); border-color: var(--text); }
      100% { box-shadow: 0 0 0 0 rgba(51, 154, 240, 0); border-color: var(--primary); }
    }
    .match.live { animation: pulse-border 2s infinite; border-width: 2px; }
    .match.live .match-header { background: var(--primary); color: white; font-weight: bold; }
    
    .match-header { font-size: 0.8rem; text-transform: uppercase; padding: 5px; text-align: center; background: #1a1a1a; color: var(--text-muted); border-bottom: 1px solid var(--border); }
    .match-team { display: flex; justify-content: space-between; padding: 10px 15px; font-size: 1.1rem; border-bottom: 1px solid var(--border); }
    .match-team:last-child { border-bottom: none; }
    .match-team.winner { font-weight: bold; color: #fff; background: rgba(51,154,240,0.1); }
    .score-box { background: #1a1a1a; padding: 2px 10px; border-radius: 4px; min-width: 20px; text-align: center; }
    .score-t1, .score-t2 { font-weight: bold; }
    
    /* Connectors Placeholder */
    .round-title { text-align: center; font-size: 1.2rem; margin-bottom: 15px; color: var(--primary); text-transform: uppercase; letter-spacing: 1px; }

    /* Tables */
    table { width: 100%; border-collapse: collapse; margin-bottom: 30px; background: #252525; border-radius: 8px; overflow: hidden; }
    th { background: #1a1a1a; color: var(--primary); text-transform: uppercase; font-size: 0.9rem; padding: 15px; }
    td { padding: 15px; border-bottom: 1px solid var(--border); font-size: 1.1rem; }
    tr:last-child td { border-bottom: none; }
    
    .list-item { display: flex; justify-content: space-between; align-items: center; padding: 15px 20px; background: #252525; margin-bottom: 10px; border-radius: 8px; border: 1px solid var(--border); transition: all 0.2s;}
    .list-item:hover { border-color: var(--primary); }
    .list-item strong { font-size: 1.2rem; }
    .big-btn { width: 100%; padding: 15px; font-size: 1.2rem; margin-top: 10px; border-radius: 8px; }

    /* Mobile Responsiveness */
    @media (max-width: 768px) {
      body { padding: 10px; }
      h1 { font-size: 1.8rem; }
      .card { padding: 15px; margin-bottom: 20px; }
      .scoreboard { flex-direction: column; gap: 10px; margin: 15px 0; }
      .team { width: 100%; padding: 15px; box-sizing: border-box; }
      .score { font-size: 4.5rem; margin: 5px 0; }
      .blue-bg, .red-bg { font-size: 1.5rem; }
      .btn-add, .btn-sub { font-size: 1.5rem; padding: 10px; }
      .scoreboard > b { font-size: 2rem !important; margin: 5px 0; display: block; text-align: center; }
      .action-buttons { flex-direction: column; gap: 10px !important; }
      .nav { gap: 8px; margin-bottom: 20px; }
      .nav button { padding: 10px; font-size: 0.9rem; flex: 1 1 30%; }
      .round { min-width: 160px; }
      .list-item { flex-direction: column; align-items: flex-start; gap: 10px; }
      .list-item > div:last-child { width: 100%; display: flex; justify-content: space-between; }
      .list-item > div:last-child button { flex: 1; }
      .match { font-size: 0.9rem; }
      .match-team { font-size: 1rem; padding: 8px 10px; }
      #b-title { font-size: 1.8rem; margin-bottom: 20px; }
    }
  </style>
</head>
<body>
  <h1>⚽ Tourney Master</h1>
  <div class="nav">
    <button onclick="showView('scoreboard')" id="nav-scoreboard">Live Screen</button>
    <button onclick="showView('tournaments')" id="nav-tournaments">Tournaments</button>
    <button onclick="showView('bracket')" id="nav-bracket" class="hidden">Bracket & Standings</button>
  </div>

  <!-- Scoreboard / Live Screen -->
  <div id="view-scoreboard" class="card">
    <h2 id="current-match-title" style="font-size: 2rem; color: var(--primary);">Casual Match</h2>
    <div class="scoreboard">
      <div class="team">
        <h2 class="blue-bg" id="team1-name">Team Blue</h2>
        <div class="score" id="score1">0</div>
        <div class="btn-group">
          <button class="btn-sub" onclick="sendCommand('t1_sub')">-1</button>
          <button class="btn-add" onclick="sendCommand('t1_add')">+1</button>
        </div>
      </div>
      <b style="font-size: 3rem; color: var(--text-muted);">VS</b>
      <div class="team">
        <h2 class="red-bg" id="team2-name">Team Red</h2>
        <div class="score" id="score2">0</div>
        <div class="btn-group">
          <button class="btn-sub" onclick="sendCommand('t2_sub')">-1</button>
          <button class="btn-add" onclick="sendCommand('t2_add')">+1</button>
        </div>
      </div>
    </div>
    <div class="action-buttons" style="display: flex; gap: 15px; margin-top: 20px;">
      <button class="big-btn" style="background:#f08c00; flex: 1;" onclick="sendCommand('reset')">Reset Scoreboard</button>
      <button id="btn-save-match" class="hidden big-btn" style="background:var(--primary); flex: 2;" onclick="saveMatchData()">Save Match & Advance</button>
    </div>
  </div>

  <!-- Tournaments List -->
  <div id="view-tournaments" class="card hidden">
    <h2>Your Tournaments</h2>
    <div id="tournament-list"></div>
    <button class="big-btn" style="background:var(--primary); margin-top:30px;" onclick="showView('create')">+ Create New Tournament</button>
  </div>

  <!-- Create Tournament View -->
  <div id="view-create" class="card hidden">
    <h2>Create New Tournament</h2>
    <label>Tournament Name:</label>
    <input type="text" id="t-name" placeholder="e.g. Friday Night Foosball Cup">
    
    <label>Mode:</label>
    <select id="t-mode">
      <option value="elimination">Elimination (Knockout Tree)</option>
      <option value="seeding">Seeding (Round Robin Only)</option>
    </select>
    
    <label>Teams (comma separated):</label>
    <input type="text" id="t-teams" placeholder="Alice/Bob, Charlie/Dave, Eve/Frank, Grace/Heidi">
    
    <button class="big-btn" style="background:var(--primary);" onclick="createTournament()">Generate & Start Tournament</button>
  </div>

  <!-- Bracket / Standings -->
  <div id="view-bracket" class="card hidden" style="max-width: 100%;">
    <h2 id="b-title" style="font-size: 2.5rem; margin-bottom: 30px;">Tournament</h2>
    <div id="seeding-section"></div>
    <div id="bracket-section" class="bracket-container"></div>
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
      
      // Live Update in Bracket
      if (activeTournament && activeMatchId) {
        let sc1 = document.getElementById(`live-s1-${activeMatchId}`);
        let sc2 = document.getElementById(`live-s2-${activeMatchId}`);
        if(sc1) sc1.innerText = data.team1;
        if(sc2) sc2.innerText = data.team2;
      }
    }
    function sendCommand(cmd) { if(websocket && websocket.readyState === WebSocket.OPEN) websocket.send(cmd); }

    // --- IndexedDB & State ---
    let db;
    let tournaments = [];
    let activeTournament = null;
    let activeMatchId = null;

    function init() {
      initWebSocket();
      let request = indexedDB.open("FoosballTournamentsPro", 1);
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

    function saveDB(tournament, callback) {
      let tx = db.transaction('tournaments', 'readwrite');
      tx.objectStore('tournaments').put(tournament);
      tx.oncomplete = () => { loadTournaments(); if(callback) callback(); };
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
      let html = tournaments.length ? '' : '<p style="text-align:center; color:gray;">No tournaments found. Create one to get started!</p>';
      // Sort newest first
      let sorted = [...tournaments].sort((a,b) => b.id - a.id);
      sorted.forEach(t => {
        html += `<div class="list-item">
          <div>
            <strong>${t.name}</strong><br>
            <small style="color:var(--text-muted)">Mode: ${t.mode} | Teams: ${t.teams.length}</small>
          </div>
          <div style="display:flex; gap:10px;">
            <button onclick="openTournament(${t.id})">Open</button>
            <button style="background:var(--accent)" onclick="deleteTournament(${t.id})">Del</button>
          </div>
        </div>`;
      });
      document.getElementById('tournament-list').innerHTML = html;
      if(tournaments.length > 0 && !activeTournament) openTournament(sorted[0].id);
      else if(tournaments.length === 0) showView('tournaments');
    }

    function deleteTournament(id) {
      if(!confirm("Are you sure you want to permanently delete this tournament?")) return;
      let tx = db.transaction('tournaments', 'readwrite');
      tx.objectStore('tournaments').delete(id);
      tx.oncomplete = () => { 
        if(activeTournament && activeTournament.id === id){ 
          activeTournament = null; activeMatchId = null; 
          document.getElementById('nav-bracket').classList.add('hidden'); 
          showView('tournaments');
        } 
        loadTournaments(); 
      };
    }

    function createTournament() {
      let name = document.getElementById('t-name').value.trim() || "Unnamed Tournament";
      let mode = document.getElementById('t-mode').value;
      let teamsRaw = document.getElementById('t-teams').value.split(',').map(s=>s.trim()).filter(s=>s);
      
      if(teamsRaw.length < 2) return alert("You need at least 2 teams to start a tournament!");

      let t = {
        id: Date.now(),
        name, mode,
        teams: teamsRaw.map((v, i) => ({ id: i, name: v, points: 0, gd: 0 })),
        matches: []
      };
      
      generateMatches(t);
      saveDB(t, () => {
        openTournament(t.id);
        document.getElementById('t-name').value = '';
        document.getElementById('t-teams').value = '';
      });
    }

    function generateMatches(t) {
      t.matches = [];
      let mIdCounter = 100;
      
      if(t.mode === 'seeding') {
        // Round Robin
        for(let i=0; i<t.teams.length; i++) {
          for(let j=i+1; j<t.teams.length; j++) {
            t.matches.push({ id: mIdCounter++, t1: t.teams[i].id, t2: t.teams[j].id, s1:0, s2:0, played:false, round:"seeding" });
          }
        }
      } else if (t.mode === 'elimination') {
        // Full Bracket Generation
        let numT = t.teams.length;
        let p2 = 1, rounds = 0;
        while(p2 < numT) { p2 *= 2; rounds++; }
        
        let localMatches = [];
        // Create empty tree
        for(let r = 0; r < rounds; r++) {
          let mInRound = p2 / Math.pow(2, r + 1);
          for(let i = 0; i < mInRound; i++) {
            let rName = r === rounds-1 ? 'Final' : (r === rounds-2 ? 'Semifinal' : 'Round ' + (r+1));
            localMatches.push({
              id: mIdCounter++, roundIndex: r, matchIndex: i, roundName: rName,
              t1: null, t2: null, s1: 0, s2: 0, played: false, nextMatchId: null, nextSlot: null
            });
          }
        }
        
        // Link tree node
        for(let m of localMatches) {
          if(m.roundIndex < rounds - 1) {
            let nextIndex = Math.floor(m.matchIndex / 2);
            let nextM = localMatches.find(x => x.roundIndex === m.roundIndex + 1 && x.matchIndex === nextIndex);
            if(nextM) {
              m.nextMatchId = nextM.id;
              m.nextSlot = (m.matchIndex % 2 === 0) ? 't1' : 't2';
            }
          }
        }
        
        // Seed first round
        let firstR = localMatches.filter(m => m.roundIndex === 0);
        let tIdx = 0;
        // Distribute teams evenly
        for(let i=0; i<firstR.length; i++) if(tIdx < numT) firstR[i].t1 = t.teams[tIdx++].id;
        for(let i=0; i<firstR.length; i++) if(tIdx < numT) firstR[i].t2 = t.teams[tIdx++].id;
        
        // Process BYEs (Matches with only 1 team)
        for(let m of firstR) {
          if((m.t1 !== null && m.t2 === null) || (m.t2 !== null && m.t1 === null)) {
            m.played = true; m.s1 = m.t1 !== null ? 1 : 0; m.s2 = m.t2 !== null ? 1 : 0;
            let adv = m.t1 !== null ? m.t1 : m.t2;
            let nextM = localMatches.find(x => x.id === m.nextMatchId);
            if(nextM) nextM[m.nextSlot] = adv;
          }
        }
        t.matches = localMatches;
      }
    }

    function openTournament(tId) {
      activeTournament = tournaments.find(t => t.id === tId);
      if(!activeTournament) return;
      document.getElementById('nav-bracket').classList.remove('hidden');
      renderActiveTournament();
      showView('bracket');
    }

    function renderActiveTournament() {
      if(!activeTournament) return;
      document.getElementById('b-title').innerText = activeTournament.name;
      
      let seedSec = document.getElementById('seeding-section');
      let brackSec = document.getElementById('bracket-section');
      seedSec.innerHTML = ''; brackSec.innerHTML = '';
      
      let teamsMap = {};
      activeTournament.teams.forEach(t => teamsMap[t.id] = t.name);

      if(activeTournament.mode === 'seeding') {
        brackSec.style.display = 'none';
        seedSec.style.display = 'block';
        
        // Calculate Standings
        activeTournament.teams.forEach(te => {te.points=0; te.gd=0;});
        activeTournament.matches.filter(m=>m.played).forEach(m => {
          let t1 = activeTournament.teams.find(x=>x.id===m.t1);
          let t2 = activeTournament.teams.find(x=>x.id===m.t2);
          if(!t1 || !t2) return;
          t1.gd += (m.s1 - m.s2); t2.gd += (m.s2 - m.s1);
          if(m.s1 > m.s2) t1.points += 3;
          else if(m.s1 < m.s2) t2.points += 3;
          else { t1.points+=1; t2.points+=1; }
        });
        
        let sHtml = "<h3>Standings</h3><table><tr><th>Team</th><th>Pts</th><th>GD</th></tr>";
        let sorted = [...activeTournament.teams].sort((a,b) => b.points - a.points || b.gd - a.gd);
        sorted.forEach(t => { sHtml += `<tr><td>${t.name}</td><td>${t.points}</td><td>${t.gd}</td></tr>`; });
        sHtml += "</table><h3 style='margin-top:30px;'>Matches</h3><div style='display:flex; flex-wrap:wrap; gap:15px; justify-content:center;'>";
        
        activeTournament.matches.forEach(m => {
          let activeClass = (m.id === activeMatchId) ? "live" : "";
          let state = m.played ? "Finished" : "Click to Play";
          let t1n = m.t1 !== null ? teamsMap[m.t1] : "TBD";
          let t2n = m.t2 !== null ? teamsMap[m.t2] : "TBD";
          let disS1 = m.id === activeMatchId ? `<span id="live-s1-${m.id}">${currentScore.team1}</span>` : m.s1;
          let disS2 = m.id === activeMatchId ? `<span id="live-s2-${m.id}">${currentScore.team2}</span>` : m.s2;
          
          let ms = m.played || m.id === activeMatchId ? `<span class="score-box score-t1">${disS1}</span> - <span class="score-box score-t2">${disS2}</span>` : `<span class="score-box">vs</span>`;
          sHtml += `<div class="match ${activeClass}" style="width:300px;" onclick="playMatch(${m.id})">
            <div class="match-header">${state}</div>
            <div class="match-team ${m.played && m.s1>m.s2 ? 'winner':''}">${t1n}</div>
            <div class="match-header" style="background:#111;">${ms}</div>
            <div class="match-team ${m.played && m.s2>m.s1 ? 'winner':''}">${t2n}</div>
          </div>`;
        });
        sHtml += "</div>";
        seedSec.innerHTML = sHtml;
      } 
      else if (activeTournament.mode === 'elimination') {
        seedSec.style.display = 'none';
        brackSec.style.display = 'flex';
        
        let roundsMap = {};
        activeTournament.matches.forEach(m => {
          if(!roundsMap[m.roundIndex]) roundsMap[m.roundIndex] = [];
          roundsMap[m.roundIndex].push(m);
        });
        
        let maxR = Math.max(...Object.keys(roundsMap).map(Number));
        let bHtml = `<div class="bracket">`;
        
        for(let r=0; r<=maxR; r++) {
          let rName = roundsMap[r][0].roundName;
          bHtml += `<div class="round"><div class="round-title">${rName}</div>`;
          
          roundsMap[r].forEach(m => {
            let activeClass = (m.id === activeMatchId) ? "live" : "";
            let t1n = m.t1 !== null ? teamsMap[m.t1] : "TBD";
            let t2n = m.t2 !== null ? teamsMap[m.t2] : "TBD";
            
            let disS1 = m.id === activeMatchId ? `<span id="live-s1-${m.id}">${currentScore.team1}</span>` : m.s1;
            let disS2 = m.id === activeMatchId ? `<span id="live-s2-${m.id}">${currentScore.team2}</span>` : m.s2;
            
            let v1Class = (m.played && m.s1 > m.s2) || (m.t1 !== null && m.t2 === null && m.played) ? "winner" : "";
            let v2Class = (m.played && m.s2 > m.s1) || (m.t2 !== null && m.t1 === null && m.played) ? "winner" : "";
            
            bHtml += `<div class="match ${activeClass}" id="match-el-${m.id}" onclick="playMatch(${m.id})">
              <div class="match-header">${m.played ? 'Finished' : (m.id === activeMatchId ? 'LIVE' : `Match ${m.id}`)}</div>
              <div class="match-team ${v1Class}"><span>${t1n}</span> <span class="score-box score-t1">${m.played || m.id===activeMatchId ? disS1 : '-'}</span></div>
              <div class="match-team ${v2Class}"><span>${t2n}</span> <span class="score-box score-t2">${m.played || m.id===activeMatchId ? disS2 : '-'}</span></div>
            </div>`;
          });
          bHtml += `</div>`;
        }
        bHtml += `</div>`;
        brackSec.innerHTML = bHtml;
      }
    }

    function playMatch(mId) {
      if(!activeTournament) return;
      let m = activeTournament.matches.find(x=>x.id===mId);
      if(!m) return;
      if(m.t1 === null || m.t2 === null) return alert("Teams are not decided for this match yet!");
      if(m.played) {
         if(!confirm("This match already finished. Play again and override?")) return;
      }
      
      activeMatchId = mId;
      document.getElementById('team1-name').innerText = activeTournament.teams.find(x=>x.id===m.t1).name;
      document.getElementById('team2-name').innerText = activeTournament.teams.find(x=>x.id===m.t2).name;
      
      let roundInfo = m.roundName ? `- ${m.roundName}` : '';
      document.getElementById('current-match-title').innerText = `${activeTournament.name} ${roundInfo}`;
      
      document.getElementById('btn-save-match').classList.remove('hidden');
      document.getElementById('btn-save-match').innerText = activeTournament.mode === 'elimination' ? "Save Match & Advance" : "Save Match";
      
      sendCommand('reset'); 
      showView('scoreboard');
      renderActiveTournament(); // Render again to show the pulsing border live state on bracket
    }

    function saveMatchData() {
      if(!activeTournament || !activeMatchId) return;
      let m = activeTournament.matches.find(x=>x.id===activeMatchId);
      
      if(currentScore.team1 === currentScore.team2 && activeTournament.mode === 'elimination') {
         return alert("Elimination matches cannot end in a draw! Keep playing.");
      }

      m.s1 = currentScore.team1;
      m.s2 = currentScore.team2;
      m.played = true;
      
      // Auto-advance in elimination
      if(activeTournament.mode === 'elimination' && m.nextMatchId) {
         let nextM = activeTournament.matches.find(x => x.id === m.nextMatchId);
         if(nextM) {
            let winnerId = m.s1 > m.s2 ? m.t1 : m.t2;
            nextM[m.nextSlot] = winnerId;
            nextM.played = false; // reset state if overriden
         }
      }

      saveDB(activeTournament, () => {
        document.getElementById('btn-save-match').classList.add('hidden');
        document.getElementById('current-match-title').innerText = "Casual Match";
        document.getElementById('team1-name').innerText = "Team Blue";
        document.getElementById('team2-name').innerText = "Team Red";
        activeMatchId = null;
        renderActiveTournament();
        showView('bracket');
      });
    }
  </script>
</body>
</html>
)rawliteral";