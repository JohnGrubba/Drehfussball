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
    players: [],
    memoryPlayers: [],
    teamDraft: [],
    draftTeams: [],
    playerFilter: '',
    selectedPlayerId: null,
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
        const req = indexedDB.open('FoosballTournamentsPro', 2);
        req.onupgradeneeded = (e) => {
          const db = e.target.result;
          if (!db.objectStoreNames.contains('tournaments')) {
            db.createObjectStore('tournaments', { keyPath: 'id' });
          }
          if (!db.objectStoreNames.contains('players')) {
            const store = db.createObjectStore('players', { keyPath: 'id' });
            store.createIndex('name', 'name', { unique: false });
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

  function dbAllPlayers() {
    return new Promise((resolve) => {
      if (!state.db) return resolve(state.memoryPlayers.map((p) => ({ ...p })));
      try {
        const tx = state.db.transaction('players', 'readonly');
        const req = tx.objectStore('players').getAll();
        req.onsuccess = () => resolve(req.result || []);
        req.onerror = () => resolve(state.memoryPlayers.map((p) => ({ ...p })));
      } catch (err) {
        resolve(state.memoryPlayers.map((p) => ({ ...p })));
      }
    });
  }

  function dbPutPlayer(p) {
    return new Promise((resolve, reject) => {
      if (!state.db) return resolve();
      try {
        const tx = state.db.transaction('players', 'readwrite');
        tx.objectStore('players').put(p);
        tx.oncomplete = () => resolve();
        tx.onerror = () => reject(tx.error);
      } catch (err) { reject(err); }
    });
  }

  function dbDeletePlayer(id) {
    return new Promise((resolve, reject) => {
      if (!state.db) return resolve();
      try {
        const tx = state.db.transaction('players', 'readwrite');
        tx.objectStore('players').delete(id);
        tx.oncomplete = () => resolve();
        tx.onerror = () => reject(tx.error);
      } catch (err) { reject(err); }
    });
  }

  function upsertMemoryPlayer(p) {
    const i = state.memoryPlayers.findIndex((x) => x.id === p.id);
    if (i >= 0) state.memoryPlayers[i] = p;
    else state.memoryPlayers.push(p);
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

  async function loadPlayers() {
    const all = await dbAllPlayers();
    state.players = Array.isArray(all) ? all : [];
    state.memoryPlayers = state.players.map((p) => ({ ...p }));
  }

  async function persistPlayer(p) {
    upsertMemoryPlayer(p);
    try {
      await dbPutPlayer(p);
    } catch (err) {
      console.warn('Spieler-DB-Fehler, nutze Speicher:', err);
      state.db = null;
    }
  }

  function normalizeName(name) {
    return String(name ?? '').trim().replace(/\s+/g, ' ');
  }

  function findPlayerByName(name) {
    const n = normalizeName(name).toLocaleLowerCase('de');
    if (!n) return null;
    return state.players.find(
      (p) => p.name.toLocaleLowerCase('de') === n) || null;
  }

  function teamPlayerIds(team) {
    if (!team) return [];
    if (Array.isArray(team.playerIds)) return team.playerIds.filter(Boolean);
    if (team.playerId) return [team.playerId];
    return [];
  }

  function teamPlayers(team) {
    return teamPlayerIds(team)
      .map((id) => state.players.find((p) => p.id === id))
      .filter(Boolean);
  }

  function teamNameFromIds(ids) {
    const names = ids
      .map((id) => state.players.find((p) => p.id === id)?.name)
      .filter(Boolean);
    return names.join(' & ') || 'Team';
  }

  function syncTeamName(team) {
    const ids = teamPlayerIds(team);
    const players = ids.map((id) => state.players.find((p) => p.id === id));
    if (!ids.length || players.some((p) => !p)) return false;
    const name = players.map((p) => p.name).join(' & ');
    if (name && team.name !== name) {
      team.name = name;
      return true;
    }
    return false;
  }

  function usedPlayerIds() {
    return new Set([
      ...state.teamDraft,
      ...state.draftTeams.flatMap((team) => team.playerIds),
    ]);
  }

  function splitTeamName(name) {
    return String(name || '')
      .split(/\s*(?:\/|&|\+)\s*/)
      .map((part) => part.trim())
      .filter(Boolean);
  }

  async function ensurePlayer(name) {
    const clean = normalizeName(name);
    if (!clean) return null;
    const existing = findPlayerByName(clean);
    if (existing) return existing;
    const player = {
      id: `p_${Date.now()}_${Math.random().toString(36).slice(2, 7)}`,
      name: clean,
      createdAt: new Date().toISOString(),
    };
    state.players.push(player);
    await persistPlayer(player);
    return player;
  }

  async function renamePlayer(id, newName) {
    const clean = normalizeName(newName);
    if (!clean) {
      toast('Der Name darf nicht leer sein.', 'error');
      return false;
    }
    const player = state.players.find((p) => p.id === id);
    if (!player) return false;
    const duplicate = state.players.find(
      (p) => p.id !== id &&
        p.name.toLocaleLowerCase('de') === clean.toLocaleLowerCase('de'));
    if (duplicate) {
      toast('Ein Spieler mit diesem Namen existiert bereits.', 'error');
      return false;
    }

    player.name = clean;
    await persistPlayer(player);

    for (const t of state.tournaments) {
      let changed = false;
      for (const team of t.teams) {
        if (teamPlayerIds(team).includes(id) && syncTeamName(team)) {
          changed = true;
        }
      }
      if (changed) {
        upsertMemory(t);
        try { await dbPut(t); } catch (err) { console.warn(err); }
      }
    }
    return true;
  }

  async function deletePlayer(id) {
    const player = state.players.find((p) => p.id === id);
    if (!player) return;
    state.players = state.players.filter((p) => p.id !== id);
    state.memoryPlayers = state.memoryPlayers.filter((p) => p.id !== id);
    state.teamDraft = state.teamDraft.filter((draftId) => draftId !== id);
    state.draftTeams = state.draftTeams.filter(
      (team) => !team.playerIds.includes(id));
    try { await dbDeletePlayer(id); } catch (err) { console.warn(err); }
    if (state.selectedPlayerId === id) state.selectedPlayerId = null;
  }

  async function migrateTournamentPlayers() {
    for (const t of state.tournaments) {
      let changed = false;
      for (const team of t.teams || []) {
        if (!Array.isArray(team.playerIds) || !team.playerIds.length) {
          let ids = [];
          if (team.playerId) {
            // Keep a stale id if the profile was deleted, so it is not
            // recreated on every load.
            ids = [team.playerId];
          } else {
            const player = await ensurePlayer(team.name);
            if (player) ids = [player.id];
          }
          team.playerIds = ids;
          changed = true;
        }
        if ('playerId' in team) {
          delete team.playerId;
          changed = true;
        }
        if (syncTeamName(team)) changed = true;
      }
      if (changed) {
        upsertMemory(t);
        try { await dbPut(t); } catch (err) { console.warn('Migration:', err); }
      }
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

  function shuffle(arr) {
    for (let i = arr.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [arr[i], arr[j]] = [arr[j], arr[i]];
    }
    return arr;
  }

  // Randomized, rest-balanced round robin.
  //
  // Circle method: if the team count is even, every "Spieltag" contains every
  // team exactly once.  With an odd team count a dummy team is added, so one
  // real team has a bye per Spieltag; the bye rotates through all teams.
  //
  // Teams, the order of the Spieltage, the order of matches inside a Spieltag
  // and home/away are randomized.  Inside a Spieltag, teams that played late
  // on the previous Spieltag are scheduled earlier, which balances the breaks.
  function generateSeedingSchedule(t) {
    t.matches = [];

    const teams = shuffle([...t.teams]);
    const slots = teams.length % 2 === 0 ? [...teams] : [...teams, null];
    const n = slots.length;
    const roundsCount = n - 1;

    let rotation = slots.slice();
    const rounds = [];

    for (let r = 0; r < roundsCount; r++) {
      const day = [];
      for (let i = 0; i < n / 2; i++) {
        const a = rotation[i];
        const b = rotation[n - 1 - i];
        if (a !== null && b !== null) day.push([a, b]);
      }
      rounds.push(day);

      // circle method: keep the first slot fixed, rotate the rest
      const fixed = rotation[0];
      const rest = rotation.slice(1);
      rest.unshift(rest.pop());
      rotation = [fixed, ...rest];
    }

    shuffle(rounds);

    const lastPlayed = new Map(teams.map((team) => [team.id, 0]));
    let id = 100;
    let sequence = 0;

    rounds.forEach((day, r) => {
      if (r === 0) {
        // first Spieltag: random order (and random first match)
        shuffle(day);
      } else {
        // Keep the relative order of the previous Spieltag: teams that
        // played early also play early here, teams that played late play
        // late.  This gives every team a similar break between days.
        day.sort((x, y) => {
          const sx = (lastPlayed.get(x[0].id) || 0) + (lastPlayed.get(x[1].id) || 0);
          const sy = (lastPlayed.get(y[0].id) || 0) + (lastPlayed.get(y[1].id) || 0);
          return sx - sy;
        });
      }

      day.forEach(([a, b], i) => {
        const flip = Math.random() < 0.5;
        t.matches.push({
          id: id++,
          t1: flip ? b.id : a.id,
          t2: flip ? a.id : b.id,
          s1: 0, s2: 0, played: false,
          round: 'seeding',
          roundIndex: r,
          matchIndex: i,
          roundName: `Spieltag ${r + 1}`,
        });
        sequence += 1;
        lastPlayed.set(a.id, sequence);
        lastPlayed.set(b.id, sequence);
      });
    });
  }

  function generateMatches(t) {
    t.matches = [];
    let id = 100;

    if (t.mode === 'seeding') {
      generateSeedingSchedule(t);
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

  // Ranking used when importing teams into a new tournament.
  // Seeding: current table. Elimination: champion, finalist, furthest round,
  // wins, goal difference. If no real match has been played, registration order.
  function tournamentRanking(t) {
    if (!t) return [];
    if (!t.matches.some((m) => m.played && !m.bye)) {
      return [...t.teams];
    }
    if (t.mode === 'seeding') {
      return standingsRows(t);
    }

    const stats = new Map();
    for (const team of t.teams) {
      stats.set(team.id, {
        team, wins: 0, losses: 0, gd: 0, reached: -1,
        champion: false, finalist: false,
      });
    }

    for (const m of t.matches) {
      const a = m.t1 !== null ? stats.get(m.t1) : null;
      const b = m.t2 !== null ? stats.get(m.t2) : null;
      const round = m.roundIndex ?? 0;
      if (a) a.reached = Math.max(a.reached, round);
      if (b) b.reached = Math.max(b.reached, round);
      if (!a || !b || !m.played) continue;

      if (m.s1 > m.s2) {
        a.wins++; b.losses++;
        a.gd += m.s1 - m.s2;
        b.gd += m.s2 - m.s1;
      } else if (m.s2 > m.s1) {
        b.wins++; a.losses++;
        b.gd += m.s2 - m.s1;
        a.gd += m.s1 - m.s2;
      }
    }

    const finalRound = t.matches.length
      ? Math.max(...t.matches.map((m) => m.roundIndex ?? 0))
      : -1;
    const final = finalRound >= 0
      ? t.matches.find((m) => (m.roundIndex ?? 0) === finalRound &&
        m.played && m.t1 !== null && m.t2 !== null)
      : null;
    if (final) {
      const champ = final.s1 > final.s2 ? final.t1
        : final.s2 > final.s1 ? final.t2 : null;
      const loser = final.s1 > final.s2 ? final.t2
        : final.s2 > final.s1 ? final.t1 : null;
      if (champ !== null) stats.get(champ).champion = true;
      if (loser !== null) stats.get(loser).finalist = true;
    }

    return [...stats.values()]
      .sort((a, b) =>
        Number(b.champion) - Number(a.champion) ||
        Number(b.finalist) - Number(a.finalist) ||
        b.reached - a.reached ||
        b.wins - a.wins ||
        b.gd - a.gd ||
        a.team.name.localeCompare(b.team.name, 'de'))
      .map((x) => x.team);
  }

  function formatAvg(value) {
    const n = Number(value) || 0;
    return n.toLocaleString('de-DE', {
      minimumFractionDigits: 2,
      maximumFractionDigits: 2,
    });
  }

  function formatDiff(value) {
    const n = Number(value) || 0;
    return n > 0 ? `+${n}` : `${n}`;
  }

  // Placement of one team in a tournament. Seeding uses the current table;
  // elimination uses champion / finalist / eliminated round.
  function tournamentPlacement(t, teamId) {
    if (!t) return { place: null, label: '–', status: '–', finished: false };

    if (t.mode === 'seeding') {
      const anyPlayed = t.matches.some((m) => m.played && !m.bye);
      if (!anyPlayed) {
        return { place: null, label: '–', status: 'Laufend', finished: false };
      }
      const rows = standingsRows(t);
      const index = rows.findIndex((r) => r.id === teamId);
      const place = index >= 0 ? index + 1 : null;
      const matches = t.matches.filter((m) => !m.bye);
      const finished = matches.length > 0 && matches.every((m) => m.played);
      return {
        place,
        label: place != null ? `${place}.` : '–',
        status: finished ? 'Beendet' : 'Laufend',
        finished,
      };
    }

    if (!t.matches.length) {
      return { place: null, label: '–', status: 'Laufend', finished: false };
    }

    const finalRound = Math.max(...t.matches.map((m) => m.roundIndex ?? 0));
    const final = t.matches.find((m) =>
      (m.roundIndex ?? 0) === finalRound && m.played &&
      m.t1 !== null && m.t2 !== null);
    const finished = !!final;
    const championId = final
      ? (final.s1 > final.s2 ? final.t1
        : final.s2 > final.s1 ? final.t2 : null)
      : null;

    if (championId === teamId) {
      return { place: 1, label: '1.', status: 'Beendet', finished: true };
    }

    let lostRound = null;
    for (const m of t.matches) {
      if (!m.played || m.t1 === null || m.t2 === null) continue;
      if (m.t1 === teamId && m.s1 < m.s2) {
        lostRound = Math.max(lostRound ?? -1, m.roundIndex ?? 0);
      } else if (m.t2 === teamId && m.s2 < m.s1) {
        lostRound = Math.max(lostRound ?? -1, m.roundIndex ?? 0);
      }
    }

    if (lostRound !== null) {
      const groupSize = 2 ** (finalRound - lostRound);
      const place = groupSize + 1;
      const label = groupSize > 1
        ? `${place}.–${place + groupSize - 1}.`
        : `${place}.`;
      return { place, label, status: finished ? 'Beendet' : 'Laufend', finished };
    }

    return { place: null, label: '–', status: 'Laufend', finished };
  }

  function computePlayerStats(playerId) {
    const history = [];
    let matches = 0;
    let wins = 0;
    let draws = 0;
    let losses = 0;
    let gf = 0;
    let ga = 0;

    for (const t of state.tournaments) {
      const team = (t.teams || []).find((x) => teamPlayerIds(x).includes(playerId));
      if (!team) continue;

      let tm = 0, tw = 0, td = 0, tl = 0, tgf = 0, tga = 0;
      for (const m of t.matches) {
        if (!m.played || m.bye || m.t1 === null || m.t2 === null) continue;
        const isT1 = m.t1 === team.id;
        const isT2 = m.t2 === team.id;
        if (!isT1 && !isT2) continue;
        const f = isT1 ? m.s1 : m.s2;
        const a = isT1 ? m.s2 : m.s1;
        tm += 1;
        tgf += f;
        tga += a;
        if (f > a) tw += 1;
        else if (f < a) tl += 1;
        else td += 1;
      }

      const placement = tournamentPlacement(t, team.id);
      const id = Number(t.id);
      const date = Number.isFinite(id)
        ? new Date(id).toLocaleDateString('de-DE')
        : '';
      history.push({
        tournamentId: t.id,
        name: t.name,
        mode: t.mode,
        date,
        teamName: team.name || teamNameFromIds(teamPlayerIds(team)),
        teamSize: teamPlayerIds(team).length,
        place: placement.place,
        placeLabel: placement.label,
        status: placement.status,
        matches: tm,
        wins: tw,
        draws: td,
        losses: tl,
        gf: tgf,
        ga: tga,
        gd: tgf - tga,
      });

      matches += tm;
      wins += tw;
      draws += td;
      losses += tl;
      gf += tgf;
      ga += tga;
    }

    history.sort((a, b) =>
      (Number(b.tournamentId) || 0) - (Number(a.tournamentId) || 0));

    const bestPlace = history.reduce((best, h) =>
      h.place != null && (best == null || h.place < best) ? h.place : best,
      null);

    return {
      tournaments: history.length,
      matches,
      wins,
      draws,
      losses,
      gf,
      ga,
      gd: gf - ga,
      avgFor: matches ? gf / matches : 0,
      avgAgainst: matches ? ga / matches : 0,
      bestPlace,
      history,
    };
  }

  function renderPlayerList(players) {
    const list = $('#player-list');
    if (!list) return;

    if (!players.length) {
      list.innerHTML = state.playerFilter.trim()
        ? '<div class="player-empty">Kein Spieler gefunden.</div>'
        : '<div class="player-empty">Noch keine Spieler. ' +
          'Lege im Setup ein Turnier an oder erstelle oben ein Profil.</div>';
      return;
    }

    list.innerHTML = players.map((p) => {
      const stats = computePlayerStats(p.id);
      const selected = p.id === state.selectedPlayerId ? ' selected' : '';
      return `<button type="button" class="player-card${selected}" data-select-player="${p.id}">` +
        `<div class="avatar" style="--hue:${playerHue(p.name)}">${playerInitials(p.name)}</div>` +
        `<div class="pc-main"><strong>${escapeHtml(p.name)}</strong>` +
        `<small>${stats.tournaments} Turnier${stats.tournaments === 1 ? '' : 'e'} · ` +
        `${stats.matches} Spiel${stats.matches === 1 ? '' : 'e'}</small></div>` +
        `<div class="pc-stats">` +
        `<span><b>${stats.gf}</b> Tore</span>` +
        `<span><b>${formatDiff(stats.gd)}</b> Diff</span>` +
        `<span><b>${formatAvg(stats.avgFor)}</b> Ø</span>` +
        `</div>` +
        `<div class="pc-best">${stats.bestPlace != null
          ? `Bestplatz ${stats.bestPlace}.`
          : 'Noch keine Platzierung'}</div>` +
        `</button>`;
    }).join('');
  }

  function renderPlayerDetail() {
    const box = $('#player-detail');
    if (!box) return;
    const player = state.players.find((p) => p.id === state.selectedPlayerId);

    if (!player) {
      box.innerHTML = '<div class="player-empty">Wähle einen Spieler aus der Liste.</div>';
      return;
    }

    const stats = computePlayerStats(player.id);
    const created = player.createdAt
      ? new Date(player.createdAt).toLocaleDateString('de-DE')
      : '';

    const history = stats.history.length
      ? `<table><thead><tr><th>Turnier</th><th>Platz</th><th>Sp</th>` +
        `<th>S</th><th>U</th><th>N</th><th>Tore</th><th>Diff</th></tr></thead><tbody>` +
        stats.history.map((h) =>
          `<tr><td><strong>${escapeHtml(h.name)}</strong>` +
          `<div class="muted small">${escapeHtml(h.date)} · ${escapeHtml(modeLabel(h.mode))}</div>` +
          (h.teamSize > 1
            ? `<div class="muted small">Team: ${escapeHtml(h.teamName)}</div>`
            : '') + `</td>` +
          `<td><span class="badge ${h.status === 'Beendet' ? 'done' : 'live'}">` +
          `${escapeHtml(h.placeLabel)}</span>` +
          `<div class="muted small">${escapeHtml(h.status)}</div></td>` +
          `<td>${h.matches}</td><td>${h.wins}</td><td>${h.draws}</td>` +
          `<td>${h.losses}</td><td>${h.gf}:${h.ga}</td>` +
          `<td>${formatDiff(h.gd)}</td></tr>`).join('') +
        `</tbody></table>`
      : '<div class="player-empty">Noch keine Turnierteilnahme.</div>';

    box.innerHTML =
      `<div class="detail-head">` +
      `<div class="avatar avatar-lg" style="--hue:${playerHue(player.name)}">${playerInitials(player.name)}</div>` +
      `<div><h3>${escapeHtml(player.name)}</h3>` +
      `<div class="muted">${created ? `Profil seit ${created}` : 'Spielerprofil'}</div></div>` +
      `</div>` +
      `<div class="detail-actions operator-only">` +
      `<button class="btn ghost small" data-player-action="rename">Umbenennen</button>` +
      `<button class="btn ghost small" data-player-action="delete">Löschen</button>` +
      `</div>` +
      `<div class="player-stat-grid">` +
      `<div class="stat-tile"><div class="label">Turniere</div><div class="value">${stats.tournaments}</div></div>` +
      `<div class="stat-tile"><div class="label">Spiele</div><div class="value">${stats.matches}</div>` +
      `<div class="sub">${stats.wins} S · ${stats.draws} U · ${stats.losses} N</div></div>` +
      `<div class="stat-tile"><div class="label">Siege</div><div class="value">${stats.wins}</div></div>` +
      `<div class="stat-tile"><div class="label">Tore</div><div class="value">${stats.gf}</div>` +
      `<div class="sub">${stats.gf}:${stats.ga}</div></div>` +
      `<div class="stat-tile"><div class="label">Tordifferenz</div><div class="value">${formatDiff(stats.gd)}</div></div>` +
      `<div class="stat-tile"><div class="label">Ø Tore</div><div class="value">${formatAvg(stats.avgFor)}</div>` +
      `<div class="sub">Ø Gegentore ${formatAvg(stats.avgAgainst)}</div></div>` +
      `</div>` +
      `<div class="player-history"><h4>Turniere</h4>${history}</div>`;
  }

  function renderPlayers() {
    const filter = $('#player-filter');
    if (filter && filter.value !== state.playerFilter) {
      state.playerFilter = filter.value;
    }
    const q = state.playerFilter.trim().toLocaleLowerCase('de');
    const players = [...state.players]
      .filter((p) => !q || p.name.toLocaleLowerCase('de').includes(q))
      .sort((a, b) => a.name.localeCompare(b.name, 'de'));

    if (state.selectedPlayerId &&
        !players.some((p) => p.id === state.selectedPlayerId)) {
      state.selectedPlayerId = players[0]?.id || null;
    }
    if (!state.selectedPlayerId && players.length) {
      state.selectedPlayerId = players[0].id;
    }

    renderPlayerList(players);
    renderPlayerDetail();
  }

  async function createPlayerProfile() {
    const input = $('#new-player-name');
    if (!input) return;
    const name = input.value.trim();
    if (!name) return;
    const existed = !!findPlayerByName(name);
    const player = await ensurePlayer(name);
    if (!player) return;
    state.selectedPlayerId = player.id;
    state.playerFilter = '';
    if ($('#player-filter')) $('#player-filter').value = '';
    input.value = '';
    renderPlayers();
    toast(existed ? 'Spieler ausgewählt.' : 'Spieler angelegt.');
  }

  async function playerAction(action, id) {
    const player = state.players.find((p) => p.id === id);
    if (!player) return;

    if (action === 'rename') {
      const name = prompt('Neuer Name', player.name);
      if (name === null) return;
      const ok = await renamePlayer(player.id, name);
      if (ok) {
        renderAll();
        toast('Spieler umbenannt.');
      }
      return;
    }

    if (action === 'delete') {
      if (!confirm(`Profil von „${player.name}“ löschen? ` +
        'Die Turnier-Einträge bleiben als Name erhalten.')) {
        return;
      }
      await deletePlayer(player.id);
      renderAll();
      toast('Profil gelöscht.');
    }
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

    const sorted = [...t.matches].sort((a, b) =>
      (a.roundIndex ?? 0) - (b.roundIndex ?? 0) ||
      (a.matchIndex ?? 0) - (b.matchIndex ?? 0) ||
      a.id - b.id);

    const groups = new Map();
    for (const m of sorted) {
      const r = m.roundIndex ?? 0;
      if (!groups.has(r)) groups.set(r, []);
      groups.get(r).push(m);
    }

    html += '<div class="matches-grid-title">Spielplan</div><div class="schedule-days">';
    for (const [r, matches] of groups) {
      const title = matches[0].roundName || (r === 0 ? 'Spiele' : `Spieltag ${r + 1}`);
      const used = new Set();
      for (const m of matches) {
        if (m.t1 !== null) used.add(m.t1);
        if (m.t2 !== null) used.add(m.t2);
      }
      const byes = t.teams.filter((team) => !used.has(team.id));
      const byeText = byes.length
        ? `<div class="matchday-bye">Freilos: ${byes.map((x) => escapeHtml(x.name)).join(', ')}</div>`
        : '';
      const legacy = matches.every((m) => m.roundIndex === undefined);
      html += `<div class="matchday${legacy ? ' legacy' : ''}"><h4>${escapeHtml(title)}</h4>${byeText}` +
        `<div class="matches-grid">${matches.map((m) => matchCard(t, m, false)).join('')}</div>` +
        `</div>`;
    }
    html += '</div>';
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
      $('#btn-shuffle-schedule').classList.add('hidden');
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

    const canShuffle = t.mode === 'seeding' && state.activeMatchId === null &&
      !t.matches.some((m) => m.played);
    $('#btn-shuffle-schedule').classList.toggle('hidden', !canShuffle);

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
  function playerHue(name) {
    let hue = 0;
    for (const char of String(name)) {
      hue = (hue * 31 + char.charCodeAt(0)) % 360;
    }
    return hue;
  }

  function playerInitials(name) {
    const parts = String(name).split(/[\s/&]+/).filter(Boolean);
    return ((parts[0]?.[0] || '?') + (parts[1]?.[0] || '')).toUpperCase();
  }

  function addTeamPlayer(player, render = true) {
    if (!player) return false;
    if (usedPlayerIds().has(player.id)) return false;
    if (state.teamDraft.length >= 2) {
      toast('Ein Team hat höchstens 2 Spieler – bitte zuerst hinzufügen.', 'error');
      return false;
    }
    state.teamDraft.push(player.id);
    if (render) {
      renderTeamBuilder();
      const input = $('#player-search');
      if (input) {
        input.value = '';
        input.focus();
      }
      hidePlayerSuggestions();
    }
    return true;
  }

  function removeTeamPlayer(id) {
    state.teamDraft = state.teamDraft.filter((x) => x !== id);
    renderTeamBuilder();
    renderPlayerSuggestions();
  }

  function addTeamToDraft() {
    if (!state.teamDraft.length) {
      toast('Bitte zuerst 1–2 Spieler für das Team auswählen.', 'error');
      return;
    }
    state.draftTeams.push({ playerIds: [...state.teamDraft] });
    state.teamDraft = [];
    renderTeamBuilder();
    renderDraftTeams();
    renderPlayerSuggestions();
    const input = $('#player-search');
    if (input) {
      input.value = '';
      input.focus();
    }
    hidePlayerSuggestions();
  }

  function removeDraftTeam(index) {
    if (index < 0 || index >= state.draftTeams.length) return;
    state.draftTeams.splice(index, 1);
    renderDraftTeams();
    renderPlayerSuggestions();
  }

  function renderTeamBuilder() {
    const box = $('#team-builder');
    if (!box) return;

    const players = state.teamDraft.map(
      (id) => state.players.find((p) => p.id === id));
    const slots = [0, 1].map((index) => {
      const player = players[index];
      if (player) {
        return `<span class="team-slot filled">` +
          `<span class="chip-avatar" style="--hue:${playerHue(player.name)}">${playerInitials(player.name)}</span>` +
          `<span class="team-slot-name">${escapeHtml(player.name)}</span>` +
          `<button type="button" data-remove-team-player="${player.id}" aria-label="Spieler entfernen">×</button>` +
          `</span>`;
      }
      return `<span class="team-slot empty">Spieler ${index + 1}</span>`;
    });

    box.innerHTML = slots.join('<span class="team-slot-plus">+</span>');
    const addBtn = $('#btn-add-team');
    if (addBtn) addBtn.disabled = !state.teamDraft.length;
  }

  function renderDraftTeams() {
    const box = $('#draft-teams');
    if (!box) return;

    if (!state.draftTeams.length) {
      box.innerHTML = '<span class="draft-empty">Noch keine Teams im Turnier.</span>';
      return;
    }

    box.innerHTML = state.draftTeams.map((team, index) => {
      const players = team.playerIds
        .map((id) => state.players.find((p) => p.id === id))
        .filter(Boolean);
      const names = players.map((player) =>
        `<span class="draft-player">` +
        `<span class="chip-avatar" style="--hue:${playerHue(player.name)}">${playerInitials(player.name)}</span>` +
        `${escapeHtml(player.name)}</span>`).join('<span class="amp">&amp;</span>');

      return `<div class="draft-team">` +
        `<div class="draft-team-players">${names}</div>` +
        `<span class="draft-team-meta">${players.length === 2 ? '2er-Team' : 'Einzel'}</span>` +
        `<button type="button" class="team-remove" data-remove-draft-team="${index}" aria-label="Team entfernen">×</button>` +
        `</div>`;
    }).join('');
  }

  function renderPlayerSuggestions() {
    const input = $('#player-search');
    const box = $('#player-suggestions');
    if (!input || !box) return;

    const query = input.value.trim();
    const ql = query.toLocaleLowerCase('de');
    const selected = usedPlayerIds();
    const all = [...state.players].sort(
      (a, b) => a.name.localeCompare(b.name, 'de'));

    if (state.teamDraft.length >= 2) {
      box.innerHTML = '<div class="suggestion muted">' +
        'Dieses Team ist voll – klicke „Team hinzufügen“.</div>';
      box.classList.remove('hidden');
      return;
    }

    let matches = all
      .filter((p) => !selected.has(p.id) &&
        (!ql || p.name.toLocaleLowerCase('de').includes(ql)));
    const exact = ql
      ? all.find((p) => p.name.toLocaleLowerCase('de') === ql)
      : null;
    if (exact && !selected.has(exact.id)) {
      matches = [exact, ...matches.filter((p) => p.id !== exact.id)];
    }
    matches = matches.slice(0, 8);

    const rows = matches.map((p) => {
      const tournaments = state.tournaments.reduce(
        (n, t) => n + (t.teams.some(
          (team) => teamPlayerIds(team).includes(p.id)) ? 1 : 0),
        0);
      const meta = tournaments
        ? `${tournaments} Turnier${tournaments === 1 ? '' : 'e'}`
        : 'Neu';
      return `<button type="button" class="suggestion" data-add-player="${p.id}">` +
        `<span class="chip-avatar" style="--hue:${playerHue(p.name)}">${playerInitials(p.name)}</span>` +
        `<span class="suggestion-name">${escapeHtml(p.name)}</span>` +
        `<span class="suggestion-meta">${meta}</span></button>`;
    }).join('');

    let extra = '';
    if (query && !exact) {
      extra = `<button type="button" class="suggestion create" data-add-new="${escapeHtml(query)}">` +
        `+ „${escapeHtml(query)}“ als neuen Spieler anlegen</button>`;
    } else if (query && exact && selected.has(exact.id)) {
      extra = `<div class="suggestion muted">„${escapeHtml(query)}“ ist bereits ausgewählt.</div>`;
    }

    if (!rows && !extra) {
      if (query) {
        box.innerHTML = '<div class="suggestion muted">Kein Treffer.</div>';
        box.classList.remove('hidden');
      } else {
        box.innerHTML = '';
        box.classList.add('hidden');
      }
      return;
    }

    if (!rows && !query && !state.players.length) {
      box.innerHTML = '<div class="suggestion muted">Noch keine Spieler. Tippe einen Namen …</div>';
      box.classList.remove('hidden');
      return;
    }

    box.innerHTML = rows + extra;
    box.classList.remove('hidden');
  }

  function hidePlayerSuggestions() {
    const box = $('#player-suggestions');
    if (box) {
      box.classList.add('hidden');
      box.innerHTML = '';
    }
  }

  function populateSourceSelect() {
    const select = $('#t-source');
    if (!select) return;
    const current = select.value;
    const sorted = [...state.tournaments].sort((a, b) => b.id - a.id);
    select.innerHTML = '<option value="">— Turnier wählen —</option>' +
      sorted.map((t) =>
        `<option value="${t.id}">${escapeHtml(t.name)} · ` +
        `${escapeHtml(modeLabel(t.mode))} · ${t.teams.length} Teams</option>`).join('');
    if (current && sorted.some((t) => String(t.id) === current)) {
      select.value = current;
    }
    updateSourceControls();
  }

  function updateSourceControls() {
    const sourceSelect = $('#t-source');
    const modeSelect = $('#t-source-mode');
    const countInput = $('#t-source-count');
    const importBtn = $('#btn-import-teams');
    const hint = $('#import-hint');
    if (!sourceSelect || !modeSelect || !countInput || !importBtn || !hint) return;

    const source = state.tournaments.find(
      (t) => String(t.id) === sourceSelect.value);
    const topMode = modeSelect.value === 'top';

    countInput.classList.toggle('hidden', !topMode);
    countInput.disabled = !source;
    importBtn.disabled = !source;

    if (!source) {
      hint.textContent = state.tournaments.length
        ? 'Wähle ein Quellturnier.'
        : 'Noch kein Quellturnier vorhanden.';
      return;
    }

    countInput.max = String(source.teams.length);
    const count = Math.min(
      Math.max(1, parseInt(countInput.value, 10) || 1),
      source.teams.length);
    hint.textContent = topMode
      ? `Übernimmt die Top ${count} von ${source.teams.length} Teams.`
      : `Übernimmt alle ${source.teams.length} Teams in Ranglisten-Reihenfolge.`;
  }

  async function resolveSourceTeamPlayerIds(team) {
    const ids = teamPlayerIds(team);
    const found = ids.map((id) => state.players.find((p) => p.id === id));
    if (ids.length >= 1 && ids.length <= 2 && found.every(Boolean)) {
      return found.map((p) => p.id);
    }

    const parts = splitTeamName(team.name);
    if (parts.length >= 1 && parts.length <= 2) {
      const players = [];
      for (const part of parts) {
        const player = await ensurePlayer(part);
        if (player) players.push(player);
      }
      if (players.length === parts.length && players.length) {
        return players.map((p) => p.id);
      }
    }

    const fallback = await ensurePlayer(team.name);
    return fallback ? [fallback.id] : [];
  }

  async function importTeamsFromSource() {
    const sourceSelect = $('#t-source');
    const source = state.tournaments.find(
      (t) => String(t.id) === sourceSelect.value);
    if (!source) {
      toast('Bitte zuerst ein Quellturnier wählen.', 'error');
      return;
    }

    const topMode = $('#t-source-mode').value === 'top';
    let count = source.teams.length;
    if (topMode) {
      count = parseInt($('#t-source-count').value, 10) || 1;
    }
    count = Math.max(1, Math.min(count, source.teams.length));

    const ranked = tournamentRanking(source).slice(0, count);
    let added = 0;
    let skipped = 0;
    for (const team of ranked) {
      const ids = await resolveSourceTeamPlayerIds(team);
      if (!ids.length) {
        skipped += 1;
        continue;
      }
      const used = usedPlayerIds();
      if (ids.some((id) => used.has(id))) {
        skipped += 1;
        continue;
      }
      state.draftTeams.push({ playerIds: ids });
      added += 1;
    }

    const search = $('#player-search');
    if (search) search.value = '';
    renderTeamBuilder();
    renderDraftTeams();
    hidePlayerSuggestions();
    if (!$('#t-name').value.trim()) {
      $('#t-name').value = `${source.name} – Neu`;
    }
    const teamWord = (n) => `${n} Team${n === 1 ? '' : 's'}`;
    toast(added
      ? `${teamWord(added)} übernommen.${skipped ? ` ${teamWord(skipped)} übersprungen.` : ''}`
      : 'Die Teams sind bereits ausgewählt.');
    updateSourceControls();
  }

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
    populateSourceSelect();
    renderTeamBuilder();
    renderDraftTeams();
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
    else if (name === 'players') renderPlayers();
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

    // Players that are already in the builder count as the next team.
    if (state.teamDraft.length) {
      state.draftTeams.push({ playerIds: [...state.teamDraft] });
      state.teamDraft = [];
      renderTeamBuilder();
      renderDraftTeams();
    }

    const draft = state.draftTeams.map((team) => ({
      playerIds: [...team.playerIds],
    }));

    if (draft.length < 2) {
      toast('Bitte mindestens 2 Teams zusammenstellen.', 'error');
      return;
    }

    if (draft.some((team) => team.playerIds.length < 1 || team.playerIds.length > 2)) {
      toast('Teams bestehen aus 1 oder 2 Spielern.', 'error');
      return;
    }

    const sizes = new Set(draft.map((team) => team.playerIds.length));
    if (sizes.size > 1) {
      toast('Bitte alle Teams mit 1 oder alle mit 2 Spielern anlegen.', 'error');
      return;
    }

    const missingPlayer = draft.some((team) =>
      team.playerIds.some((id) => !state.players.some((p) => p.id === id)));
    if (missingPlayer) {
      toast('Ein Spieler im Team existiert nicht mehr. Bitte Team prüfen.', 'error');
      return;
    }

    const t = {
      id: Date.now(),
      name,
      mode,
      teams: draft.map((team, i) => ({
        id: i,
        name: teamNameFromIds(team.playerIds),
        playerIds: [...team.playerIds],
      })),
      matches: [],
    };
    generateMatches(t);
    await persist(t);
    state.activeTournament = t;
    state.activeMatchId = null;
    state.teamDraft = [];
    state.draftTeams = [];
    $('#t-name').value = '';
    $('#player-search').value = '';
    hidePlayerSuggestions();
    renderTeamBuilder();
    renderDraftTeams();
    renderAll();
    showView('tournament');
    toast('Turnier erstellt.');
  }

  async function seedDemo(open = true) {
    let t = state.tournaments.find((x) => x.demo && x.name === DEMO_NAME);
    if (!t) {
      const teams = [];
      for (const name of DEMO_TEAMS) {
        const p = await ensurePlayer(name);
        if (p) teams.push({ id: teams.length, name: p.name, playerIds: [p.id] });
      }
      t = {
        id: Date.now(),
        name: DEMO_NAME,
        mode: 'elimination',
        demo: true,
        teams,
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

  async function shuffleSchedule() {
    const t = state.activeTournament;
    if (!t || t.mode !== 'seeding') return;
    if (state.activeMatchId !== null) {
      toast('Bitte zuerst das laufende Spiel speichern oder zurücksetzen.', 'error');
      return;
    }
    if (t.matches.some((m) => m.played)) {
      toast('Der Spielplan kann nur vor dem ersten Spiel neu gemischt werden.', 'error');
      return;
    }
    if (!confirm('Spielplan neu mischen? Die Paarungen bleiben gleich, nur die Reihenfolge ändert sich.')) {
      return;
    }
    generateSeedingSchedule(t);
    await persist(t);
    state.activeTournament = state.tournaments.find((x) => x.id === t.id) || t;
    renderAll();
    toast('Spielplan neu gemischt.');
  }

  function playMatch(id) {
    if (state.tv) return;
    const t = state.activeTournament;
    if (!t) return;
    const m = t.matches.find((x) => x.id === id);
    if (!m) return;
    if (m.bye) return;
    if (m.t1 === null || m.t2 === null) {
      toast('Die Spieler für dieses Spiel stehen noch nicht fest.', 'error');
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
    $('#btn-shuffle-schedule').addEventListener('click', shuffleSchedule);
    $('#btn-generate').addEventListener('click', createTournament);
    $('#t-source').addEventListener('change', updateSourceControls);
    $('#t-source-mode').addEventListener('change', updateSourceControls);
    $('#t-source-count').addEventListener('input', updateSourceControls);
    $('#btn-import-teams').addEventListener('click', importTeamsFromSource);
    $('#btn-add-team').addEventListener('click', addTeamToDraft);
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

    $('#t-name').addEventListener('keydown', (e) => {
      if (e.key === 'Enter') createTournament();
    });

    $('#player-search').addEventListener('input', renderPlayerSuggestions);
    $('#player-search').addEventListener('focus', renderPlayerSuggestions);
    $('#player-search').addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        hidePlayerSuggestions();
        return;
      }
      if (e.key !== 'Enter') return;
      e.preventDefault();
      const box = $('#player-suggestions');
      const first = box && !box.classList.contains('hidden')
        ? box.querySelector('button')
        : null;
      if (first) {
        first.click();
        return;
      }
      const name = $('#player-search').value.trim();
      if (name) {
        ensurePlayer(name).then((p) => { if (p) addTeamPlayer(p); });
      }
    });

    $('#btn-create-player').addEventListener('click', createPlayerProfile);
    $('#new-player-name').addEventListener('keydown', (e) => {
      if (e.key === 'Enter') createPlayerProfile();
    });
    $('#player-filter').addEventListener('input', () => {
      state.playerFilter = $('#player-filter').value;
      renderPlayers();
    });

    document.addEventListener('click', (e) => {
      const addPlayerBtn = e.target.closest('[data-add-player]');
      if (addPlayerBtn) {
        e.preventDefault();
        const player = state.players.find(
          (p) => p.id === addPlayerBtn.dataset.addPlayer);
        addTeamPlayer(player);
        return;
      }

      const addNewBtn = e.target.closest('[data-add-new]');
      if (addNewBtn) {
        e.preventDefault();
        const name = addNewBtn.dataset.addNew;
        ensurePlayer(name).then((p) => { if (p) addTeamPlayer(p); });
        return;
      }

      const removeTeamPlayerBtn = e.target.closest('[data-remove-team-player]');
      if (removeTeamPlayerBtn) {
        e.preventDefault();
        removeTeamPlayer(removeTeamPlayerBtn.dataset.removeTeamPlayer);
        return;
      }

      const removeDraftTeamBtn = e.target.closest('[data-remove-draft-team]');
      if (removeDraftTeamBtn) {
        e.preventDefault();
        removeDraftTeam(Number(removeDraftTeamBtn.dataset.removeDraftTeam));
        return;
      }

      const selectPlayerBtn = e.target.closest('[data-select-player]');
      if (selectPlayerBtn) {
        e.preventDefault();
        state.selectedPlayerId = selectPlayerBtn.dataset.selectPlayer;
        renderPlayers();
        return;
      }

      const playerActionBtn = e.target.closest('[data-player-action]');
      if (playerActionBtn) {
        e.preventDefault();
        if (state.selectedPlayerId) {
          playerAction(playerActionBtn.dataset.playerAction, state.selectedPlayerId);
        }
        return;
      }

      if (!e.target.closest('.player-picker')) hidePlayerSuggestions();

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
    renderPlayers();
    updateScoreUI();
    updateMockButton();
    updateConnNote();
  }

  async function init() {
    await openDB();
    await loadPlayers();
    await loadTournaments(true);
    await migrateTournamentPlayers();

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
