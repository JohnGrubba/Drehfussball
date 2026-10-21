/* =========================================================================
   Tourney Master — dashboard logic
   Works with the ESP32 WebSocket and with the local mock server.
   ========================================================================= */

(() => {
  'use strict';

  // ── tiny helpers ────────────────────────────────────────────────────────
  const $ = (sel, root = document) => root.querySelector(sel);
  const $$ = (sel, root = document) => [...root.querySelectorAll(sel)];
  const clampInt = (v) => {
    const n = Number(v);
    return Number.isFinite(n) ? Math.max(0, Math.min(255, Math.trunc(n))) : 0;
  };
  const escapeHtml = (value = '') =>
    String(value).replace(/[&<>"']/g, (c) => ({
      '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
    }[c]));

  const params = new URLSearchParams(location.search);
  const IS_FILE = location.protocol === 'file:';
  const IS_TV_PATH = location.pathname.replace(/\/+$/, '') === '/tv';
  const DEMO_NAME = 'Friday Night Foosball Cup';
  const DEMO_TEAMS = [
    'Die Ballermänner', 'FC Tischkante', 'Team Titan', 'Die Roten',
    'Stangenfest', 'Ballzauber', 'Netzroller', 'Kellerkinder',
  ];

  // ── state ───────────────────────────────────────────────────────────────
  const state = {
    db: null,
    memory: [],
    tournaments: [],
    activeTournament: null,
    activeMatchId: null,
    currentScore: { team1: 0, team2: 0 },
    view: 'live',
    tv: false,
    mock: params.has('mock') || IS_FILE,
    ws: null,
    wsStatus: 'connecting',
    reconnectTimer: null,
    firstScore: true,
    goalTimer: null,
    toastTimer: null,
  };

  // ── IndexedDB ───────────────────────────────────────────────────────────
  function openDB() {
    return new Promise((resolve) => {
      if (!('indexedDB' in window)) return resolve(false);
      try {
        const req = indexedDB.open('FoosballTournamentsPro', 1);
        req.onupgradeneeded = (e) => {
          const db = e.target.result;
          if (!db.objectStoreNames.contains('tournaments')) {
            db.createObjectStore('tournaments', { keyPath: 'id' });
          }
        };
        req.onsuccess = (e) => { state.db = e.target.result; resolve(true); };
        req.onerror = () => { state.db = null; resolve(false); };
        req.onblocked = () => { state.db = null; resolve(false); };
      } catch (err) {
        console.warn('IndexedDB nicht verfügbar:', err);
        state.db = null;
        resolve(false);
      }
    });
  }

  function dbAll() {
    return new Promise((resolve) => {
      if (!state.db) return resolve(state.memory.map((t) => ({ ...t })));
      try {
        const tx = state.db.transaction('tournaments', 'readonly');
        const req = tx.objectStore('tournaments').getAll();
        req.onsuccess = () => resolve(req.result || []);
        req.onerror = () => resolve(state.memory.map((t) => ({ ...t })));
      } catch (err) {
        resolve(state.memory.map((t) => ({ ...t })));
      }
    });
  }

  function dbPut(t) {
    return new Promise((resolve, reject) => {
      if (!state.db) return resolve();
      try {
        const tx = state.db.transaction('tournaments', 'readwrite');
        tx.objectStore('tournaments').put(t);
        tx.oncomplete = () => resolve();
        tx.onerror = () => reject(tx.error);
        tx.onabort = () => reject(tx.error);
      } catch (err) { reject(err); }
    });
  }

  function dbDelete(id) {
    return new Promise((resolve, reject) => {
      if (!state.db) return resolve();
      try {
        const tx = state.db.transaction('tournaments', 'readwrite');
        tx.objectStore('tournaments').delete(id);
        tx.oncomplete = () => resolve();
        tx.onerror = () => reject(tx.error);
      } catch (err) { reject(err); }
    });
  }

  function upsertMemory(t) {
    const i = state.memory.findIndex((x) => x.id === t.id);
    if (i >= 0) state.memory[i] = t;
    else state.memory.push(t);
  }

  async function loadTournaments(selectNewest = true) {
    const all = await dbAll();
    state.tournaments = all.map((t) => ({
      ...t,
      matches: Array.isArray(t.matches) ? t.matches : [],
    }));

    const activeId = state.activeTournament?.id;
    if (activeId != null) {
      state.activeTournament =
        state.tournaments.find((t) => t.id === activeId) || null;
    }
    if (!state.activeTournament && selectNewest && state.tournaments.length) {
      state.activeTournament = [...state.tournaments]
        .sort((a, b) => b.id - a.id)[0];
    }
    if (state.activeTournament && state.activeMatchId != null &&
        !state.activeTournament.matches.some((m) => m.id === state.activeMatchId)) {
      state.activeMatchId = null;
    }
  }

  async function persist(t) {
    upsertMemory(t);
    try {
      await dbPut(t);
    } catch (err) {
      console.warn('DB-Schreibfehler, nutze Speicher:', err);
      state.db = null;
    }
    const activeId = state.activeTournament?.id;
    state.tournaments = await dbAll();
    if (activeId != null) {
      state.activeTournament =
        state.tournaments.find((x) => x.id === activeId) || t;
    } else {
      state.activeTournament = t;
    }
  }

  // ── tournament model ────────────────────────────────────────────────────
  const modeLabel = (m) =>
    m === 'seeding' ? 'Jeder gegen jeden' : 'K.-o.-System';

  const roundLabel = (m) =>
    m.roundName || (m.round === 'seeding' ? 'Jeder gegen jeden' : 'Spiel');

  function roundName(r, rounds) {
    const fromEnd = rounds - r;
    if (fromEnd === 1) return 'Finale';
    if (fromEnd === 2) return 'Halbfinale';
    if (fromEnd === 3) return 'Viertelfinale';
    if (fromEnd === 4) return 'Achtelfinale';
    return `Runde ${r + 1}`;
  }

  function generateMatches(t) {
    t.matches = [];
    let id = 100;

    if (t.mode === 'seeding') {
      for (let i = 0; i < t.teams.length; i++) {
        for (let j = i + 1; j < t.teams.length; j++) {
          t.matches.push({
            id: id++, t1: t.teams[i].id, t2: t.teams[j].id,
            s1: 0, s2: 0, played: false,
            round: 'seeding', roundName: 'Jeder gegen jeden',
          });
        }
      }
      return;
    }

    const n = t.teams.length;
    let p2 = 1;
    let rounds = 0;
    while (p2 < n) { p2 *= 2; rounds++; }

    const local = [];
    for (let r = 0; r < rounds; r++) {
      const count = p2 / Math.pow(2, r + 1);
      for (let i = 0; i < count; i++) {
        local.push({
          id: id++, roundIndex: r, matchIndex: i,
          roundName: roundName(r, rounds),
          t1: null, t2: null, s1: 0, s2: 0,
          played: false, bye: false,
          nextMatchId: null, nextSlot: null,
        });
      }
    }

    for (const m of local) {
      if (m.roundIndex < rounds - 1) {
        const nextIndex = Math.floor(m.matchIndex / 2);
        const next = local.find(
          (x) => x.roundIndex === m.roundIndex + 1 && x.matchIndex === nextIndex);
        if (next) {
          m.nextMatchId = next.id;
          m.nextSlot = (m.matchIndex % 2 === 0) ? 't1' : 't2';
        }
      }
    }

    const first = local.filter((m) => m.roundIndex === 0);
    let idx = 0;
    for (const m of first) if (idx < n) m.t1 = t.teams[idx++].id;
    for (const m of first) if (idx < n) m.t2 = t.teams[idx++].id;

    for (const m of first) {
      if ((m.t1 !== null && m.t2 === null) ||
          (m.t2 !== null && m.t1 === null)) {
        m.played = true;
        m.bye = true;
        m.s1 = m.t1 !== null ? 1 : 0;
        m.s2 = m.t2 !== null ? 1 : 0;
        const winner = m.t1 !== null ? m.t1 : m.t2;
        const next = local.find((x) => x.id === m.nextMatchId);
        if (next) next[m.nextSlot] = winner;
      }
    }

    t.matches = local;
  }

  function advanceWinner(t, m) {
    if (!m || m.nextMatchId == null) return;
    const next = t.matches.find((x) => x.id === m.nextMatchId);
    if (!next) return;
    const winner = m.s1 > m.s2 ? m.t1 : m.t2;
    if (winner !== null) next[m.nextSlot] = winner;
  }

  function clearAdvance(t, m) {
    if (!m) return;
    m.played = false;
    m.s1 = 0;
    m.s2 = 0;
    m.bye = false;
    if (m.nextMatchId != null) {
      const next = t.matches.find((x) => x.id === m.nextMatchId);
      if (next) {
        next[m.nextSlot] = null;
        clearAdvance(t, next);
      }
    }
  }

  const teamName = (t, id) => {
    const team = t?.teams?.find((x) => x.id === id);
    return team ? team.name : 'TBD';
  };

  function activeMatch() {
    const t = state.activeTournament;
    if (!t || state.activeMatchId == null) return null;
    return t.matches.find((m) => m.id === state.activeMatchId) || null;
  }

  function playedCount(t) {
    const total = t.matches.filter((m) => !m.bye).length;
    const played = t.matches.filter((m) => m.played && !m.bye).length;
    return { played, total };
  }

  // ── rendering: header / live ────────────────────────────────────────────
  function renderHeader() {
    const t = state.activeTournament;
    const m = activeMatch();
    $('#brand-sub').textContent = t ? t.name : 'Casual Match';

    const n1 = m ? teamName(t, m.t1) : 'Blau';
    const n2 = m ? teamName(t, m.t2) : 'Rot';
    $('#mini-t1').textContent = n1;
    $('#mini-t2').textContent = n2;
    $('#mini-t1').title = n1;
    $('#mini-t2').title = n2;
  }

  function renderLive() {
    const t = state.activeTournament;
    const m = activeMatch();

    const n1 = m ? teamName(t, m.t1) : 'Team Blue';
    const n2 = m ? teamName(t, m.t2) : 'Team Red';

    const el1 = $('#live-name1');
    const el2 = $('#live-name2');
    el1.textContent = n1;
    el2.textContent = n2;
    el1.title = n1;
    el2.title = n2;

    $('#match-status').textContent = m
      ? `${t.name} · ${roundLabel(m)}`
      : 'CASUAL MATCH';

    const saveBtn = $('#btn-save-match');
    if (m) {
      saveBtn.classList.remove('hidden');
      saveBtn.textContent = t.mode === 'elimination'
        ? 'Spiel speichern & weiter'
        : 'Spiel speichern';
    } else {
      saveBtn.classList.add('hidden');
    }

    const openBtn = $('#btn-open-tournament');
    openBtn.textContent = t ? 'Turnierübersicht' : 'Turnier anlegen';
  }

  function updateScoreUI() {
    const s1 = state.currentScore.team1;
    const s2 = state.currentScore.team2;
    $('#score1').textContent = s1;
    $('#score2').textContent = s2;
    $('#mini-s1').textContent = s1;
    $('#mini-s2').textContent = s2;
    $$('[data-live="s1"]').forEach((el) => { el.textContent = s1; });
    $$('[data-live="s2"]').forEach((el) => { el.textContent = s2; });
  }

  // ── rendering: tournament ───────────────────────────────────────────────
  function matchCard(t, m, showRound = true) {
    const live = m.id === state.activeMatchId;
    const finished = !!m.played;
    const tbd = m.t1 === null || m.t2 === null;
    const draw = finished && m.s1 === m.s2;
    const winnerId = finished && !draw
      ? (m.s1 > m.s2 ? m.t1 : m.t2)
      : null;

    const classes = ['match'];
    if (live) classes.push('live');
    if (finished) classes.push('finished');
    if (tbd) classes.push('tbd');
    if (m.bye) classes.push('bye');

    const header = live
      ? '<span class="live-tag">● LIVE</span>'
      : m.bye
        ? 'Freilos'
        : finished
          ? '<span class="win-tag">✓ Beendet</span>'
          : 'Offen';

    const row = (id, score, side) => {
      const known = id !== null;
      const name = known ? teamName(t, id) : 'TBD';
      const isWinner = winnerId !== null && id === winnerId;
      const isLoser = winnerId !== null && id !== null && id !== winnerId;
      const showScore = finished || live;
      const scoreHtml = showScore
        ? `<span class="score-box"${live ? ` data-live="${side}"` : ''}>${score}</span>`
        : '<span class="score-box">–</span>';
      return `<div class="team-row${isWinner ? ' winner' : ''}${isLoser ? ' loser' : ''}">` +
        `<span class="name${known ? '' : ' tbd-text'}">${escapeHtml(name)}</span>` +
        `${scoreHtml}</div>`;
    };

    const score1 = live ? state.currentScore.team1 : m.s1;
    const score2 = live ? state.currentScore.team2 : m.s2;

    return `<div class="${classes.join(' ')}" data-match="${m.id}"` +
      `${!tbd && !state.tv ? ' title="Klicken zum Starten"' : ''}>` +
      `<div class="match-header"><span>${header}</span>` +
      `${showRound ? `<span>${escapeHtml(roundLabel(m))}</span>` : ''}</div>` +
      `${row(m.t1, score1, 's1')}${row(m.t2, score2, 's2')}` +
      `</div>`;
  }

  function renderBracket(t) {
    const rounds = new Map();
    for (const m of t.matches) {
      const r = m.roundIndex ?? 0;
      if (!rounds.has(r)) rounds.set(r, []);
      rounds.get(r).push(m);
    }
    const keys = [...rounds.keys()].sort((a, b) => a - b);
    return keys.map((r) => {
      const matches = rounds.get(r).sort((a, b) => a.matchIndex - b.matchIndex);
      const title = matches[0]?.roundName || `Runde ${r + 1}`;
      return `<div class="round"><div class="round-title">${escapeHtml(title)}</div>` +
        `<div class="round-matches">${matches.map((m) => matchCard(t, m, false)).join('')}</div>` +
        `</div>`;
    }).join('');
  }

  function standingsRows(t) {
    const rows = new Map(t.teams.map((team) => [team.id, {
      ...team, played: 0, wins: 0, draws: 0, losses: 0,
      gf: 0, ga: 0, gd: 0, points: 0,
    }]));

    for (const m of t.matches) {
      if (!m.played || m.bye || m.t1 === null || m.t2 === null) continue;
      const a = rows.get(m.t1);
      const b = rows.get(m.t2);
      if (!a || !b) continue;
      a.played++; b.played++;
      a.gf += m.s1; a.ga += m.s2;
      b.gf += m.s2; b.ga += m.s1;
      if (m.s1 > m.s2) { a.wins++; b.losses++; a.points += 3; }
      else if (m.s1 < m.s2) { b.wins++; a.losses++; b.points += 3; }
      else { a.draws++; b.draws++; a.points++; b.points++; }
    }

    const list = [...rows.values()];
    for (const r of list) r.gd = r.gf - r.ga;
    list.sort((a, b) =>
      b.points - a.points || b.gd - a.gd || b.gf - a.gf ||
      a.name.localeCompare(b.name, 'de'));
    return list;
  }

  function renderStandings(t) {
    const rows = standingsRows(t);
    let html = '<h3>Tabelle</h3><table><thead><tr>' +
      '<th>#</th><th>Team</th><th>Sp</th><th>S</th><th>U</th><th>N</th>' +
      '<th>Tore</th><th>Diff</th><th>Pkt</th>' +
      '</tr></thead><tbody>';

    rows.forEach((r, i) => {
      const diff = r.gd > 0 ? `+${r.gd}` : `${r.gd}`;
      html += `<tr class="${i === 0 ? 'leader' : ''}">` +
        `<td class="rank">${i + 1}</td>` +
        `<td>${escapeHtml(r.name)}</td>` +
        `<td>${r.played}</td><td>${r.wins}</td><td>${r.draws}</td><td>${r.losses}</td>` +
        `<td>${r.gf}:${r.ga}</td><td class="gd">${diff}</td>` +
        `<td class="pts">${r.points}</td></tr>`;
    });
    html += '</tbody></table>';

    const sorted = [...t.matches].sort((a, b) => a.id - b.id);
    html += '<div class="matches-grid-title">Spiele</div><div class="matches-grid">' +
      sorted.map((m) => matchCard(t, m)).join('') + '</div>';
    return html;
  }

  function renderSidebar(t) {
    const liveCard = $('#live-card');
    const nextList = $('#next-matches');

    if (!t) {
      liveCard.innerHTML = '<div class="side-title">LIVE</div>' +
        '<div class="empty-live">Kein Turnier geladen.</div>';
      nextList.innerHTML = '<div class="next-empty">Kein Turnier geladen.</div>';
      return;
    }

    const m = activeMatch();
    if (m) {
      liveCard.innerHTML = '<div class="side-title">LIVE</div>' +
        `<div class="live-team blue"><span>${escapeHtml(teamName(t, m.t1))}</span>` +
        `<b data-live="s1">${state.currentScore.team1}</b></div>` +
        `<div class="live-team red"><span>${escapeHtml(teamName(t, m.t2))}</span>` +
        `<b data-live="s2">${state.currentScore.team2}</b></div>` +
        `<div class="live-round">${escapeHtml(roundLabel(m))}</div>` +
        '<button class="btn ghost small" data-view="live">Zum Live-Screen</button>';
    } else {
      liveCard.innerHTML = '<div class="side-title">LIVE</div>' +
        '<div class="empty-live">Kein aktives Spiel.<br>Wähle ein offenes Spiel im Bracket.</div>';
    }

    const next = t.matches
      .filter((x) => !x.played && !x.bye && x.t1 !== null && x.t2 !== null &&
        x.id !== state.activeMatchId)
      .sort((a, b) =>
        (a.roundIndex ?? 0) - (b.roundIndex ?? 0) ||
        (a.matchIndex ?? 0) - (b.matchIndex ?? 0) ||
        a.id - b.id)
      .slice(0, 6);

    if (!next.length) {
      nextList.innerHTML = '<div class="next-empty">Keine offenen Spiele.</div>';
      return;
    }

    nextList.innerHTML = next.map((x) =>
      `<div class="next-item" data-match="${x.id}">` +
      `<span class="next-round">${escapeHtml(roundLabel(x))}</span>` +
      `<div class="next-teams"><span>${escapeHtml(teamName(t, x.t1))}</span>` +
      `<span class="vs-text">vs</span>` +
      `<span>${escapeHtml(teamName(t, x.t2))}</span></div></div>`).join('');
  }

  function renderTournament() {
    const t = state.activeTournament;
    const bracket = $('#bracket');
    const standings = $('#standings');
    const empty = $('#empty-tournament');
    const champion = $('#champion');

    if (!t) {
      $('#t-title').textContent = 'Kein Turnier';
      $('#t-meta').textContent = 'Erstelle ein Turnier im Setup.';
      champion.classList.add('hidden');
      bracket.classList.add('hidden');
      standings.classList.add('hidden');
      empty.classList.remove('hidden');
      bracket.innerHTML = '';
      standings.innerHTML = '';
      renderSidebar(null);
      return;
    }

    const { played, total } = playedCount(t);
    $('#t-title').textContent = t.name;
    $('#t-meta').textContent =
      `${modeLabel(t.mode)} · ${t.teams.length} Teams · ${played}/${total} Spiele`;

    // champion (elimination only)
    let champ = null;
    if (t.mode === 'elimination' && t.matches.length) {
      const maxRound = Math.max(...t.matches.map((m) => m.roundIndex ?? 0));
      const final = t.matches.find((m) => (m.roundIndex ?? 0) === maxRound);
      if (final && final.played && final.t1 !== null && final.t2 !== null) {
        champ = teamName(t, final.s1 > final.s2 ? final.t1 : final.t2);
      }
    }
    if (champ) {
      champion.innerHTML = `🏆 ${escapeHtml(champ)}`;
      champion.classList.remove('hidden');
    } else {
      champion.classList.add('hidden');
    }

    empty.classList.add('hidden');

    if (t.mode === 'seeding') {
      bracket.classList.add('hidden');
      standings.classList.remove('hidden');
      standings.innerHTML = renderStandings(t);
    } else {
      standings.classList.add('hidden');
      bracket.classList.remove('hidden');
      bracket.innerHTML = renderBracket(t);
    }

    renderSidebar(t);
  }

  // ── rendering: setup ────────────────────────────────────────────────────
  function renderSetup() {
    const list = $('#tournament-list');
    const sorted = [...state.tournaments].sort((a, b) => b.id - a.id);

    if (!sorted.length) {
      list.innerHTML = '<div class="empty-list">Noch keine Turniere. ' +
        'Erstelle eins oder lade das Demo-Turnier.</div>';
    } else {
      list.innerHTML = sorted.map((t) => {
        const { played, total } = playedCount(t);
        return `<div class="tournament-item">` +
          `<div class="ti-main"><strong>${escapeHtml(t.name)}</strong>` +
          `<small>${escapeHtml(modeLabel(t.mode))} · ${t.teams.length} Teams · ` +
          `${played}/${total} Spiele</small></div>` +
          `<div class="ti-actions">` +
          `<button class="btn ghost small" data-action="open" data-id="${t.id}">Öffnen</button>` +
          `<button class="btn ghost small" data-action="delete" data-id="${t.id}">Löschen</button>` +
          `</div></div>`;
      }).join('');
    }
    updateMockButton();
    updateConnNote();
  }

  function updateMockButton() {
    const btn = $('#btn-mock');
    if (!btn) return;
    if (IS_FILE) {
      btn.textContent = 'Demo-Modus aktiv (file://)';
      btn.disabled = true;
      return;
    }
    btn.disabled = false;
    btn.textContent = state.mock
      ? 'Demo-Modus beenden'
      : 'Demo-Modus (ohne ESP32)';
  }

  // ── connection ──────────────────────────────────────────────────────────
  function wsUrl() {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    return `${proto}//${location.host}/ws`;
  }

  function setConn(status, title) {
    state.wsStatus = status;
    const el = $('#conn');
    el.className = `conn ${status}`;
    el.title = title;
    updateConnNote();
  }

  function updateConnNote() {
    const note = $('#connection-note');
    if (!note) return;
    const text = {
      online: 'Live mit dem ESP32 verbunden.',
      offline: 'Offline – läuft der lokale Server?',
      connecting: 'Verbinde mit dem ESP32 …',
      demo: 'Demo-Modus – kein ESP32 verbunden.',
    }[state.wsStatus] || '';
    note.textContent = text;
  }

  function scheduleReconnect() {
    if (state.mock) return;
    clearTimeout(state.reconnectTimer);
    state.reconnectTimer = setTimeout(connectWS, 2500);
  }

  function connectWS() {
    if (state.mock) {
      startMock();
      return;
    }
    clearTimeout(state.reconnectTimer);
    if (state.ws) {
      try { state.ws.close(); } catch (err) { /* ignore */ }
    }
    setConn('connecting', 'Verbinde …');
    let ws;
    try {
      ws = new WebSocket(wsUrl());
    } catch (err) {
      setConn('offline', 'Offline');
      scheduleReconnect();
      return;
    }
    state.ws = ws;
    state.firstScore = true;

    ws.onopen = () => {
      setConn('online', 'Live verbunden');
      state.firstScore = true;
    };
    ws.onmessage = (ev) => {
      let data;
      try { data = JSON.parse(ev.data); } catch (err) { return; }
      if (typeof data.team1 !== 'number' || typeof data.team2 !== 'number') return;
      const silent = state.firstScore;
      state.firstScore = false;
      applyScore(data, silent);
    };
    ws.onclose = () => {
      setConn('offline', 'Offline');
      scheduleReconnect();
    };
    ws.onerror = () => setConn('offline', 'Verbindungsfehler');
  }

  function sendCommand(cmd) {
    if (state.ws && state.ws.readyState === 1) {
      state.ws.send(cmd);
      return true;
    }
    toast('Nicht verbunden – läuft der lokale Server?', 'error');
    return false;
  }

  function applyScore(data, silent = false) {
    const prev = state.currentScore;
    const next = { team1: clampInt(data.team1), team2: clampInt(data.team2) };
    state.currentScore = next;
    updateScoreUI();
    if (silent) return;
    const blue = next.team1 > prev.team1;
    const red = next.team2 > prev.team2;
    if (blue || red) {
      goalFlash(blue && red ? 'both' : blue ? 'blue' : 'red');
    }
  }

  // ── mock socket ─────────────────────────────────────────────────────────
  class MockSocket {
    constructor() {
      this.readyState = 1;
      this.score = { team1: 0, team2: 0 };
      setTimeout(() => {
        this.onopen?.();
        this.emit();
      }, 60);
    }
    send(cmd) {
      if (cmd === 't1_add') this.score.team1 = Math.min(255, this.score.team1 + 1);
      else if (cmd === 't1_sub') this.score.team1 = Math.max(0, this.score.team1 - 1);
      else if (cmd === 't2_add') this.score.team2 = Math.min(255, this.score.team2 + 1);
      else if (cmd === 't2_sub') this.score.team2 = Math.max(0, this.score.team2 - 1);
      else if (cmd === 'reset') this.score = { team1: 0, team2: 0 };
      else return;
      this.emit();
    }
    emit() {
      const snapshot = { ...this.score };
      setTimeout(() => {
        this.onmessage?.({ data: JSON.stringify(snapshot) });
      }, 30);
    }
    close() {
      this.readyState = 3;
    }
  }

  function startMock() {
    state.firstScore = true;
    const mock = new MockSocket();
    state.ws = mock;
    setConn('demo', 'Demo-Modus');
    mock.onopen = () => setConn('demo', 'Demo-Modus');
    mock.onmessage = (ev) => {
      let data;
      try { data = JSON.parse(ev.data); } catch (err) { return; }
      const silent = state.firstScore;
      state.firstScore = false;
      applyScore(data, silent);
    };
  }

  // ── goal animation / toast ──────────────────────────────────────────────
  function goalFlash(side) {
    const panels = [];
    if (side === 'blue' || side === 'both') panels.push($('#panel-1'));
    if (side === 'red' || side === 'both') panels.push($('#panel-2'));

    for (const panel of panels) {
      if (!panel) continue;
      panel.classList.remove('goal');
      void panel.offsetWidth;
      panel.classList.add('goal');
      setTimeout(() => panel.classList.remove('goal'), 1100);
    }

    const overlay = $('#goal-overlay');
    overlay.style.color = side === 'blue'
      ? 'var(--blue)'
      : side === 'red'
        ? 'var(--red)'
        : '#fff';
    overlay.classList.remove('show');
    void overlay.offsetWidth;
    overlay.classList.add('show');
    clearTimeout(state.goalTimer);
    state.goalTimer = setTimeout(() => overlay.classList.remove('show'), 1200);
  }

  function toast(message, type = 'info') {
    const el = $('#toast');
    el.textContent = message;
    el.className = `toast show${type === 'error' ? ' error' : ''}`;
    clearTimeout(state.toastTimer);
    state.toastTimer = setTimeout(() => el.classList.remove('show'), 2800);
  }

  // ── views / actions ─────────────────────────────────────────────────────
  function showView(name) {
    state.view = name;
    $$('.view').forEach((v) => {
      v.classList.toggle('active', v.id === `view-${name}`);
    });
    $$('.nav-btn').forEach((b) => {
      b.classList.toggle('active', b.dataset.view === name);
    });
    if (name === 'tournament') renderTournament();
    else if (name === 'setup') renderSetup();
    else renderLive();
  }

  function setTV(on) {
    state.tv = !!on;
    document.body.classList.toggle('tv', state.tv);
    $('#btn-tv').classList.toggle('active', state.tv);
    if (state.tv && state.view === 'setup') {
      showView(state.activeTournament ? 'tournament' : 'live');
    }
  }

  async function toggleFullscreen() {
    try {
      if (!document.fullscreenElement) {
        await document.documentElement.requestFullscreen();
      } else {
        await document.exitFullscreen();
      }
    } catch (err) {
      toast('Vollbild ist in diesem Browser nicht verfügbar.', 'error');
    }
  }

  function openTournament(id) {
    const t = state.tournaments.find((x) => x.id === id);
    if (!t) return;
    state.activeTournament = t;
    state.activeMatchId = null;
    showView('tournament');
    renderAll();
  }

  async function deleteTournament(id) {
    if (!confirm('Dieses Turnier wirklich löschen?')) return;
    try {
      await dbDelete(id);
    } catch (err) {
      console.warn(err);
    }
    state.memory = state.memory.filter((t) => t.id !== id);
    if (state.activeTournament?.id === id) {
      state.activeTournament = null;
      state.activeMatchId = null;
    }
    await loadTournaments(true);
    renderAll();
    showView('setup');
    toast('Turnier gelöscht.');
  }

  async function createTournament() {
    const name = $('#t-name').value.trim() || 'Unbenanntes Turnier';
    const mode = $('#t-mode').value;
    const teamsRaw = $('#t-teams').value
      .split(',')
      .map((s) => s.trim())
      .filter(Boolean);

    if (teamsRaw.length < 2) {
      toast('Bitte mindestens 2 Teams eingeben.', 'error');
      return;
    }

    const t = {
      id: Date.now(),
      name,
      mode,
      teams: teamsRaw.map((n, i) => ({ id: i, name: n })),
      matches: [],
    };
    generateMatches(t);
    await persist(t);
    state.activeTournament = t;
    state.activeMatchId = null;
    $('#t-name').value = '';
    $('#t-teams').value = '';
    renderAll();
    showView('tournament');
    toast('Turnier erstellt.');
  }

  async function seedDemo(open = true) {
    let t = state.tournaments.find((x) => x.demo && x.name === DEMO_NAME);
    if (!t) {
      t = {
        id: Date.now(),
        name: DEMO_NAME,
        mode: 'elimination',
        demo: true,
        teams: DEMO_TEAMS.map((n, i) => ({ id: i, name: n })),
        matches: [],
      };
      generateMatches(t);

      const first = t.matches
        .filter((m) => m.roundIndex === 0)
        .sort((a, b) => a.matchIndex - b.matchIndex);

      const results = [
        [10, 7], [6, 10], [10, 3],
      ];
      results.forEach(([s1, s2], i) => {
        const m = first[i];
        if (!m) return;
        m.s1 = s1;
        m.s2 = s2;
        m.played = true;
        advanceWinner(t, m);
      });

      await persist(t);
      t = state.tournaments.find((x) => x.id === t.id) || t;
    }

    state.activeTournament = t;
    state.activeMatchId = null;
    if (open) {
      renderAll();
      showView('tournament');
      toast('Demo-Turnier geladen.');
    }
    return t;
  }

  function playMatch(id) {
    if (state.tv) return;
    const t = state.activeTournament;
    if (!t) return;
    const m = t.matches.find((x) => x.id === id);
    if (!m) return;
    if (m.bye) return;
    if (m.t1 === null || m.t2 === null) {
      toast('Die Teams für dieses Spiel stehen noch nicht fest.', 'error');
      return;
    }
    if (m.played) {
      if (!confirm('Dieses Spiel wurde bereits gespielt. Neu starten und überschreiben?')) {
        return;
      }
    }

    state.activeMatchId = id;
    state.currentScore = { team1: 0, team2: 0 };
    state.firstScore = true;
    updateScoreUI();

    sendCommand('reset');
    setTimeout(() => { state.firstScore = false; }, 1200);

    renderAll();
    showView('live');
  }

  async function saveMatchData() {
    const t = state.activeTournament;
    const m = activeMatch();
    if (!t || !m) return;

    if (t.mode === 'elimination' &&
        state.currentScore.team1 === state.currentScore.team2) {
      toast('K.-o.-Spiele können nicht unentschieden enden.', 'error');
      return;
    }

    m.s1 = state.currentScore.team1;
    m.s2 = state.currentScore.team2;
    m.played = true;
    m.bye = false;

    if (t.mode === 'elimination' && m.nextMatchId != null) {
      const next = t.matches.find((x) => x.id === m.nextMatchId);
      if (next) {
        next[m.nextSlot] = m.s1 > m.s2 ? m.t1 : m.t2;
        clearAdvance(t, next);
      }
    }

    await persist(t);
    state.activeMatchId = null;
    renderAll();
    showView('tournament');
    toast('Spiel gespeichert.');
  }

  // ── bindings ────────────────────────────────────────────────────────────
  function bind() {
    $('#btn-save-match').addEventListener('click', saveMatchData);
    $('#btn-generate').addEventListener('click', createTournament);
    $('#btn-setup-demo').addEventListener('click', () => seedDemo(true));
    $('#btn-new-tournament').addEventListener('click', () => {
      showView('setup');
      $('#create-card')?.scrollIntoView({ behavior: 'smooth', block: 'start' });
      setTimeout(() => $('#t-name')?.focus(), 200);
    });
    $('#btn-open-tournament').addEventListener('click', () => {
      showView(state.activeTournament ? 'tournament' : 'setup');
    });
    $('#btn-tv').addEventListener('click', () => setTV(!state.tv));
    $('#btn-fullscreen').addEventListener('click', toggleFullscreen);
    $('#btn-mock').addEventListener('click', () => {
      const url = new URL(location.href);
      if (state.mock) url.searchParams.delete('mock');
      else url.searchParams.set('mock', '1');
      location.href = url.toString();
    });

    for (const id of ['#t-name', '#t-teams']) {
      $(id).addEventListener('keydown', (e) => {
        if (e.key === 'Enter') createTournament();
      });
    }

    document.addEventListener('click', (e) => {
      const viewBtn = e.target.closest('[data-view]');
      if (viewBtn) {
        e.preventDefault();
        showView(viewBtn.dataset.view);
        return;
      }

      const cmdBtn = e.target.closest('[data-cmd]');
      if (cmdBtn) {
        e.preventDefault();
        sendCommand(cmdBtn.dataset.cmd);
        return;
      }

      const actionBtn = e.target.closest('[data-action]');
      if (actionBtn) {
        e.preventDefault();
        const id = Number(actionBtn.dataset.id);
        if (actionBtn.dataset.action === 'open') openTournament(id);
        else if (actionBtn.dataset.action === 'delete') deleteTournament(id);
        return;
      }

      const matchEl = e.target.closest('[data-match]');
      if (matchEl) {
        e.preventDefault();
        playMatch(Number(matchEl.dataset.match));
      }
    });

    document.addEventListener('keydown', (e) => {
      if (e.target.closest('input, select, textarea')) return;
      const key = e.key.toLowerCase();

      if (key === 't') { setTV(!state.tv); return; }
      if (key === 'f') { toggleFullscreen(); return; }
      if (e.key === 'Escape' && state.tv) { setTV(false); return; }
      if (state.tv) return;

      if (e.key === '1') sendCommand('t1_add');
      else if (e.key === '2') sendCommand('t2_add');
      else if (key === 'r') sendCommand('reset');
    });
  }

  // ── main render ─────────────────────────────────────────────────────────
  function renderAll() {
    renderHeader();
    renderLive();
    renderTournament();
    renderSetup();
    updateScoreUI();
    updateMockButton();
    updateConnNote();
  }

  async function init() {
    await openDB();
    await loadTournaments(true);

    if (params.has('demo')) {
      const demo = await seedDemo(false);
      state.activeTournament = demo;
      state.activeMatchId = null;
    }

    setTV(params.has('tv') || IS_TV_PATH);
    bind();

    if (params.has('setup')) {
      showView('setup');
    } else if (params.has('demo') || params.has('tv') || IS_TV_PATH) {
      showView('tournament');
    } else {
      showView('live');
    }

    renderAll();
    connectWS();
  }

  // expose a tiny debug handle (useful for local preview / screenshots)
  window.__TM = { state, showView, seedDemo, setTV, renderAll };

  init().catch((err) => {
    console.error(err);
    toast('Dashboard konnte nicht initialisiert werden.', 'error');
  });
})();
