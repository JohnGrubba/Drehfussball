#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="theme-color" content="#070b10">
  <title>Tourney Master</title>
  <style>
/* =========================================================================
   Tourney Master — large-screen dashboard
   ========================================================================= */

:root {
  --bg: #070b10;
  --bg-2: #0b1119;
  --panel: #0f1620;
  --panel-2: #141e2a;
  --panel-3: #1a2634;
  --border: #22303f;
  --text: #e9f0f7;
  --muted: #8b9bb0;
  --blue: #3ea6ff;
  --blue-2: #1c7ed6;
  --red: #ff5c5c;
  --red-2: #e03131;
  --green: #37b24d;
  --amber: #f59f00;
  --gold: #ffd43b;
  --radius: 20px;
  --shadow: 0 24px 70px rgba(0, 0, 0, .45);
  --font: system-ui, -apple-system, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
  --mono: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
}

* { box-sizing: border-box; }

html, body { height: 100%; }

body {
  margin: 0;
  background:
    radial-gradient(1200px 800px at 12% -10%, rgba(62, 166, 255, .16), transparent 60%),
    radial-gradient(1000px 700px at 92% 110%, rgba(255, 92, 92, .13), transparent 60%),
    var(--bg);
  color: var(--text);
  font-family: var(--font);
  overflow: hidden;
  -webkit-font-smoothing: antialiased;
  text-rendering: optimizeLegibility;
}

body::before {
  content: "";
  position: fixed;
  inset: 0;
  pointer-events: none;
  background-image:
    linear-gradient(rgba(255, 255, 255, .025) 1px, transparent 1px),
    linear-gradient(90deg, rgba(255, 255, 255, .025) 1px, transparent 1px);
  background-size: 56px 56px;
  mask-image: radial-gradient(ellipse at center, black 30%, transparent 78%);
  z-index: 0;
}

button, input, select { font: inherit; }
button { color: inherit; }
.hidden { display: none !important; }

/* ── app shell ─────────────────────────────────────────────────────────── */

.app {
  position: relative;
  z-index: 1;
  height: 100vh;
  height: 100dvh;
  display: flex;
  flex-direction: column;
}

.topbar {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  padding: 14px clamp(16px, 2vw, 30px);
  background: rgba(7, 11, 16, .72);
  border-bottom: 1px solid var(--border);
  backdrop-filter: blur(16px);
}

.brand { display: flex; align-items: center; gap: 12px; min-width: 0; }

.brand-ball {
  width: 44px;
  height: 44px;
  display: grid;
  place-items: center;
  border-radius: 14px;
  background: linear-gradient(145deg, var(--blue-2), #0b4f8a);
  box-shadow: 0 8px 24px rgba(62, 166, 255, .3);
  font-size: 24px;
  flex: 0 0 auto;
}

.brand-text { min-width: 0; }
.brand h1 {
  margin: 0;
  font-size: clamp(1rem, 1.35vw, 1.35rem);
  letter-spacing: .12em;
  font-weight: 800;
  white-space: nowrap;
}
.brand h1 span { color: var(--blue); }
.brand-sub {
  margin-top: 2px;
  color: var(--muted);
  font-size: clamp(.7rem, .8vw, .85rem);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 42vw;
}

.live-mini {
  display: flex;
  align-items: center;
  gap: clamp(8px, 1.2vw, 18px);
  padding: 8px clamp(12px, 1.4vw, 20px);
  border: 1px solid var(--border);
  border-radius: 999px;
  background: rgba(20, 30, 42, .72);
  font-variant-numeric: tabular-nums;
}
.mini-side { display: flex; align-items: baseline; gap: 10px; min-width: 0; }
.mini-side.blue .mini-name { color: var(--blue); }
.mini-side.red .mini-name { color: var(--red); }
.mini-name {
  font-size: clamp(.72rem, .85vw, .95rem);
  font-weight: 700;
  max-width: 14vw;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.mini-score { font-size: clamp(1.1rem, 1.6vw, 1.6rem); font-weight: 800; line-height: 1; }
.mini-colon { color: var(--muted); font-weight: 800; }

.nav { display: flex; align-items: center; gap: 8px; }
.nav-btn, .icon-btn {
  border: 1px solid transparent;
  background: transparent;
  color: var(--muted);
  padding: 9px 14px;
  border-radius: 10px;
  cursor: pointer;
  font-weight: 700;
  font-size: clamp(.75rem, .85vw, .9rem);
  transition: background .15s, color .15s, border-color .15s, transform .15s;
}
.nav-btn:hover, .icon-btn:hover { color: var(--text); background: rgba(255, 255, 255, .06); }
.nav-btn.active {
  color: #fff;
  background: linear-gradient(145deg, var(--blue-2), #0b5ca8);
  box-shadow: 0 6px 18px rgba(28, 126, 214, .28);
}
.icon-btn {
  border-color: var(--border);
  font-family: var(--mono);
  font-size: .8rem;
  letter-spacing: .04em;
}
.icon-btn.active { color: #001018; background: var(--blue); border-color: var(--blue); }
.nav-sep { width: 1px; height: 24px; background: var(--border); margin: 0 4px; }

.conn {
  font-size: 14px;
  line-height: 1;
  margin-left: 4px;
  color: var(--amber);
  transition: color .2s, text-shadow .2s;
}
.conn.online { color: var(--green); text-shadow: 0 0 12px rgba(55, 178, 77, .8); }
.conn.offline { color: var(--red); text-shadow: 0 0 12px rgba(255, 92, 92, .7); }
.conn.demo { color: #b197fc; text-shadow: 0 0 12px rgba(177, 151, 252, .7); }

/* ── views ─────────────────────────────────────────────────────────────── */

.main { position: relative; flex: 1; min-height: 0; }
.view { display: none; height: 100%; }
.view.active { display: flex; }

/* ── live ──────────────────────────────────────────────────────────────── */

.view-live {
  position: relative;
  padding: clamp(14px, 2.2vw, 44px);
}

.live-stage {
  flex: 1;
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(190px, .55fr) minmax(0, 1fr);
  gap: clamp(14px, 2.2vw, 42px);
}

.team-panel {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: clamp(8px, 1.5vh, 24px);
  min-height: 0;
  padding: clamp(18px, 2.4vw, 42px);
  border: 1px solid rgba(255, 255, 255, .08);
  border-radius: clamp(18px, 2vw, 34px);
  background:
    linear-gradient(165deg, var(--team-soft, rgba(255, 255, 255, .04)), transparent 48%),
    linear-gradient(180deg, var(--panel-3), var(--panel));
  overflow: hidden;
  overflow: clip;
  overflow-anchor: none;
  isolation: isolate;
  box-shadow: var(--shadow);
}
.team-panel::after {
  content: "";
  position: absolute;
  inset: auto -20% -45% -20%;
  height: 70%;
  background: radial-gradient(circle at 50% 50%, var(--team-glow), transparent 65%);
  opacity: .55;
  z-index: -1;
}
.team-panel.blue {
  --team: var(--blue);
  --team-soft: rgba(62, 166, 255, .16);
  --team-glow: rgba(62, 166, 255, .28);
  border-color: rgba(62, 166, 255, .22);
}
.team-panel.red {
  --team: var(--red);
  --team-soft: rgba(255, 92, 92, .15);
  --team-glow: rgba(255, 92, 92, .26);
  border-color: rgba(255, 92, 92, .22);
}

.team-kicker {
  color: var(--team);
  font-size: clamp(.62rem, .78vw, .85rem);
  font-weight: 800;
  letter-spacing: .24em;
  text-transform: uppercase;
}
.team-title {
  margin: 0;
  max-width: 100%;
  text-align: center;
  font-size: clamp(1.45rem, 2.7vw, 3.6rem);
  line-height: 1.05;
  overflow-wrap: anywhere;
}
.score {
  color: var(--team);
  font-size: clamp(6.5rem, 17vw, 20rem);
  line-height: .82;
  font-weight: 900;
  font-variant-numeric: tabular-nums;
  letter-spacing: -.04em;
  text-shadow: 0 0 70px var(--team-glow);
  user-select: none;
}

.operator-controls { display: flex; gap: clamp(10px, 1vw, 18px); }
.score-btn {
  width: clamp(58px, 6.4vw, 104px);
  height: clamp(58px, 6.4vw, 104px);
  border-radius: 50%;
  border: 1px solid rgba(255, 255, 255, .14);
  background: rgba(255, 255, 255, .07);
  color: #fff;
  font-size: clamp(1.4rem, 2vw, 2.4rem);
  font-weight: 800;
  cursor: pointer;
  transition: transform .12s, background .15s, box-shadow .15s;
}
.score-btn:hover { transform: translateY(-2px); background: rgba(255, 255, 255, .14); }
.score-btn:active { transform: translateY(0) scale(.96); }
.score-btn.add { background: var(--team); color: #06121d; border-color: transparent; box-shadow: 0 10px 28px var(--team-glow); }
.score-btn.sub { color: var(--team); }

.live-center {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: clamp(10px, 1.8vh, 24px);
  min-width: 0;
  text-align: center;
}
.match-status {
  color: var(--muted);
  font-size: clamp(.65rem, .8vw, .9rem);
  font-weight: 800;
  letter-spacing: .18em;
  text-transform: uppercase;
  max-width: 100%;
  line-height: 1.35;
  overflow-wrap: anywhere;
}
.vs {
  font-size: clamp(2.4rem, 5vw, 6rem);
  font-weight: 900;
  color: rgba(255, 255, 255, .18);
  letter-spacing: .04em;
  line-height: 1;
}
.center-actions { display: flex; flex-direction: column; gap: 10px; width: min(100%, 260px); }
.hint { color: var(--muted); font-size: clamp(.65rem, .72vw, .78rem); line-height: 1.6; }
kbd {
  display: inline-block;
  padding: 1px 5px;
  border: 1px solid var(--border);
  border-bottom-width: 2px;
  border-radius: 5px;
  background: var(--panel-2);
  font-family: var(--mono);
  font-size: .82em;
}

/* buttons */
.btn {
  border: 1px solid transparent;
  border-radius: 11px;
  padding: 11px 16px;
  font-weight: 700;
  cursor: pointer;
  background: var(--panel-3);
  color: var(--text);
  transition: transform .12s, filter .15s, background .15s, border-color .15s;
  white-space: nowrap;
}
.btn:hover { transform: translateY(-1px); filter: brightness(1.1); }
.btn:active { transform: translateY(0); }
.btn.primary { background: linear-gradient(145deg, var(--blue-2), #0b5ca8); color: #fff; box-shadow: 0 8px 24px rgba(28, 126, 214, .28); }
.btn.warn { background: rgba(245, 159, 0, .14); color: var(--amber); border-color: rgba(245, 159, 0, .3); }
.btn.ghost { background: transparent; border-color: var(--border); color: var(--muted); }
.btn.ghost:hover { color: var(--text); border-color: #3b4d61; }
.btn.big { padding: 14px 18px; font-size: 1rem; }
.btn.small { padding: 7px 11px; font-size: .78rem; }

/* goal flash */
.team-panel.goal { animation: teamGoal 1.05s ease; }
@keyframes teamGoal {
  0% { box-shadow: 0 0 0 0 var(--team-glow); }
  25% { box-shadow: 0 0 0 12px transparent, 0 0 90px 18px var(--team-glow); }
  100% { box-shadow: var(--shadow); }
}
.goal-overlay {
  position: absolute;
  inset: 0;
  z-index: 20;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: clamp(5rem, 18vw, 22rem);
  font-weight: 900;
  letter-spacing: .02em;
  color: #fff;
  background: rgba(3, 7, 12, .55);
  backdrop-filter: blur(3px);
  opacity: 0;
  pointer-events: none;
}
.goal-overlay.show { animation: goalPop 1.15s cubic-bezier(.2, .8, .2, 1) forwards; }
@keyframes goalPop {
  0% { opacity: 0; transform: scale(.6); }
  18% { opacity: 1; transform: scale(1.04); }
  70% { opacity: 1; transform: scale(1); }
  100% { opacity: 0; transform: scale(1.08); }
}

/* ── tournament ────────────────────────────────────────────────────────── */

.view-tournament {
  flex-direction: column;
  gap: clamp(10px, 1.4vh, 18px);
  padding: clamp(12px, 1.8vw, 30px);
}

.tournament-head {
  flex: 0 0 auto;
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: 18px;
}
.tournament-heading { min-width: 0; }
.tournament-head h2 {
  margin: 0;
  font-size: clamp(1.35rem, 2.1vw, 2.5rem);
  line-height: 1.1;
  overflow-wrap: anywhere;
}
.t-meta { margin-top: 5px; color: var(--muted); font-size: clamp(.78rem, .95vw, 1.05rem); }
.champion {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
  padding: 6px 12px;
  border: 1px solid rgba(255, 212, 59, .35);
  border-radius: 999px;
  background: rgba(255, 212, 59, .1);
  color: var(--gold);
  font-weight: 800;
  font-size: clamp(.78rem, .95vw, 1rem);
}
.tournament-actions { display: flex; gap: 8px; flex: 0 0 auto; }

.tournament-body {
  flex: 1;
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(0, 1fr) clamp(290px, 24vw, 460px);
  gap: clamp(10px, 1.4vw, 20px);
}

.bracket-wrap {
  position: relative;
  min-height: 0;
  overflow: auto;
  border: 1px solid var(--border);
  border-radius: var(--radius);
  background:
    linear-gradient(180deg, rgba(20, 30, 42, .72), rgba(10, 16, 24, .72));
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, .04);
}
.bracket {
  display: flex;
  align-items: stretch;
  gap: clamp(14px, 1.8vw, 34px);
  min-height: 100%;
  padding: clamp(12px, 1.4vw, 22px);
}
.round {
  display: flex;
  flex-direction: column;
  flex: 1;
  min-width: clamp(170px, 12.5vw, 280px);
  min-height: 0;
}
.round-title {
  position: sticky;
  top: 0;
  z-index: 3;
  padding: 4px 2px 10px;
  color: var(--muted);
  font-size: clamp(.6rem, .73vw, .8rem);
  font-weight: 800;
  letter-spacing: .2em;
  text-transform: uppercase;
  background: linear-gradient(180deg, rgba(13, 20, 29, .96), rgba(13, 20, 29, .82) 70%, transparent);
}
.round-matches {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: space-around;
  gap: clamp(8px, 1.1vh, 18px);
  min-height: 0;
}

.match {
  position: relative;
  padding: clamp(7px, .85vw, 13px);
  border: 1px solid var(--border);
  border-radius: 12px;
  background: linear-gradient(180deg, var(--panel-3), var(--panel-2));
  cursor: pointer;
  transition: border-color .15s, transform .15s, box-shadow .15s, opacity .15s;
  overflow: hidden;
}
.match:hover { border-color: rgba(62, 166, 255, .55); transform: translateY(-1px); box-shadow: 0 10px 28px rgba(0, 0, 0, .28); }
.match.finished { opacity: .86; }
.match.tbd { opacity: .58; cursor: default; }
.match.tbd:hover { border-color: var(--border); transform: none; box-shadow: none; }
.match.live {
  border-color: rgba(62, 166, 255, .85);
  box-shadow: 0 0 0 1px rgba(62, 166, 255, .35), 0 0 34px rgba(62, 166, 255, .22);
  animation: livePulse 2.2s ease-in-out infinite;
}
@keyframes livePulse {
  0%, 100% { box-shadow: 0 0 0 1px rgba(62, 166, 255, .35), 0 0 24px rgba(62, 166, 255, .16); }
  50% { box-shadow: 0 0 0 1px rgba(62, 166, 255, .8), 0 0 42px rgba(62, 166, 255, .3); }
}
.match-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  padding-bottom: 6px;
  margin-bottom: 4px;
  border-bottom: 1px solid rgba(255, 255, 255, .05);
  color: var(--muted);
  font-size: clamp(.56rem, .66vw, .72rem);
  font-weight: 700;
  letter-spacing: .08em;
  text-transform: uppercase;
}
.match-header .live-tag { color: var(--blue); }
.match-header .win-tag { color: var(--green); }
.team-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: clamp(4px, .55vh, 7px) 0;
  font-size: clamp(.78rem, .95vw, 1.08rem);
  color: var(--muted);
}
.team-row + .team-row { border-top: 1px solid rgba(255, 255, 255, .045); }
.team-row .name { min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.team-row .score-box {
  flex: 0 0 auto;
  min-width: 2.1em;
  padding: 1px 7px;
  border-radius: 6px;
  background: rgba(0, 0, 0, .3);
  color: var(--text);
  font-weight: 800;
  font-variant-numeric: tabular-nums;
  text-align: center;
}
.team-row.winner { color: var(--text); font-weight: 800; }
.team-row.winner .score-box { color: var(--green); background: rgba(55, 178, 77, .13); }
.team-row.loser .score-box { color: var(--muted); }
.match.live .team-row .score-box { color: var(--blue); }
.tbd-text { color: var(--muted); font-style: italic; }

/* standings / round robin */
.standings { padding: clamp(12px, 1.6vw, 24px); min-height: 100%; }
.standings h3, .matches-grid-title {
  margin: 0 0 12px;
  font-size: clamp(1rem, 1.2vw, 1.3rem);
}
.standings table { width: 100%; border-collapse: collapse; }
.standings th {
  padding: 10px 12px;
  text-align: left;
  color: var(--muted);
  font-size: clamp(.65rem, .75vw, .8rem);
  letter-spacing: .14em;
  text-transform: uppercase;
  border-bottom: 1px solid var(--border);
}
.standings td {
  padding: clamp(8px, 1vh, 14px) 12px;
  border-bottom: 1px solid rgba(255, 255, 255, .055);
  font-size: clamp(.9rem, 1.1vw, 1.25rem);
}
.standings tr:last-child td { border-bottom: none; }
.standings .rank { color: var(--muted); width: 3ch; font-variant-numeric: tabular-nums; }
.standings th:nth-child(n+3), .standings td:nth-child(n+3) { text-align: right; }
.standings .pts { color: var(--blue); font-weight: 800; font-variant-numeric: tabular-nums; }
.standings .gd { font-variant-numeric: tabular-nums; }
.standings .leader { background: rgba(62, 166, 255, .07); }

.matches-grid-title { margin-top: clamp(18px, 2.4vh, 32px); }
.schedule-days {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
  gap: 14px;
  align-items: start;
}
.matchday { margin-top: 0; }
.matchday .matches-grid { grid-template-columns: 1fr; gap: 10px; }
.matchday.legacy { grid-column: 1 / -1; }
.matchday.legacy .matches-grid { grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); }
.matchday .match { padding: 8px 10px; }
.matchday .team-row { font-size: clamp(.72rem, .8vw, .95rem); padding: 4px 0; }
.matchday-bye {
  margin: -4px 0 8px;
  color: var(--muted);
  font-size: .72rem;
}
.matchday h4 {
  margin: 0 0 6px;
  color: var(--muted);
  font-size: .72rem;
  font-weight: 800;
  letter-spacing: .16em;
  text-transform: uppercase;
}
.matches-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(clamp(220px, 18vw, 320px), 1fr));
  gap: 12px;
}

/* sidebar */
.sidebar {
  display: flex;
  flex-direction: column;
  gap: clamp(10px, 1.2vw, 16px);
  min-height: 0;
  overflow: auto;
}
.side-card {
  padding: clamp(12px, 1.4vw, 20px);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  background: linear-gradient(180deg, var(--panel-2), var(--panel));
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, .04);
}
.side-title {
  color: var(--muted);
  font-size: clamp(.6rem, .72vw, .78rem);
  font-weight: 800;
  letter-spacing: .18em;
  text-transform: uppercase;
  margin-bottom: 10px;
}
.live-card .side-title { color: var(--blue); }
.live-card .live-team {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 7px 0;
  font-size: clamp(.9rem, 1.15vw, 1.3rem);
  font-weight: 700;
}
.live-card .live-team + .live-team { border-top: 1px solid rgba(255, 255, 255, .06); }
.live-card .live-team.blue { color: var(--blue); }
.live-card .live-team.red { color: var(--red); }
.live-card .live-team b { font-size: 1.5em; font-variant-numeric: tabular-nums; }
.live-card .live-round { margin-top: 8px; color: var(--muted); font-size: .85em; }
.live-card .empty-live { color: var(--muted); line-height: 1.5; }

.next-list { display: flex; flex-direction: column; gap: 8px; }
.next-item:hover { border-color: rgba(62, 166, 255, .45); transform: translateY(-1px); }
.next-item {
  padding: 9px 10px;
  cursor: pointer;
  transition: border-color .15s, transform .15s;

  border: 1px solid rgba(255, 255, 255, .06);
  border-radius: 10px;
  background: rgba(0, 0, 0, .14);
}
.next-round { display: block; color: var(--muted); font-size: .7rem; text-transform: uppercase; letter-spacing: .1em; margin-bottom: 3px; }
.next-teams { display: flex; justify-content: space-between; gap: 8px; font-size: clamp(.78rem, .9vw, .98rem); }
.next-teams .vs-text { color: var(--muted); font-size: .8em; align-self: center; }
.next-empty { color: var(--muted); font-size: .9rem; line-height: 1.5; }

/* ── setup ─────────────────────────────────────────────────────────────── */

.view-setup {
  flex-direction: column;
  gap: 18px;
  padding: clamp(16px, 2vw, 32px);
  overflow: auto;
}
.setup-head {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: 18px;
  flex-wrap: wrap;
}
.setup-head h2 { margin: 0; font-size: clamp(1.4rem, 2vw, 2rem); }
.muted { color: var(--muted); }
.setup-head p { margin: 4px 0 0; }
.setup-actions { display: flex; gap: 8px; flex-wrap: wrap; }

.setup-grid {
  display: grid;
  grid-template-columns: minmax(0, 1.1fr) minmax(0, .9fr);
  gap: 18px;
  align-items: start;
}
.card {
  padding: clamp(16px, 1.8vw, 26px);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  background: linear-gradient(180deg, var(--panel-2), var(--panel));
  box-shadow: var(--shadow);
}
.card h3 { margin: 0 0 16px; font-size: clamp(1.05rem, 1.2vw, 1.3rem); }
.card label {
  display: block;
  margin: 12px 0 6px;
  color: var(--muted);
  font-size: .78rem;
  font-weight: 800;
  letter-spacing: .12em;
  text-transform: uppercase;
}
.card input, .card select {
  width: 100%;
  padding: 11px 12px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #0a1119;
  color: var(--text);
  outline: none;
  transition: border-color .15s, box-shadow .15s;
}
.card input:focus, .card select:focus { border-color: var(--blue); box-shadow: 0 0 0 3px rgba(62, 166, 255, .14); }
.fine { color: var(--muted); font-size: .78rem; line-height: 1.5; }

.import-box {
  margin: 14px 0 18px;
  padding: 14px;
  border: 1px solid var(--border);
  border-radius: 12px;
  background: rgba(0, 0, 0, .16);
}
.import-title {
  margin-bottom: 10px;
  color: var(--muted);
  font-size: .72rem;
  font-weight: 800;
  letter-spacing: .14em;
  text-transform: uppercase;
}
.import-box label { margin-top: 0; }
.import-row {
  display: flex;
  gap: 8px;
  align-items: center;
  margin-top: 8px;
}
.import-row select, .import-row input[type="number"] { width: auto; }
.import-row select { flex: 1 1 auto; min-width: 0; }
.import-row input[type="number"] { flex: 0 0 88px; }
.import-row .btn { flex: 0 0 auto; }
.import-box .fine { margin: 8px 0 0; }

.tournament-list { display: flex; flex-direction: column; gap: 10px; }
.tournament-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 14px;
  border: 1px solid var(--border);
  border-radius: 12px;
  background: rgba(0, 0, 0, .14);
}
.tournament-item .ti-main { min-width: 0; }
.tournament-item strong { display: block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.tournament-item small { color: var(--muted); }
.tournament-item .ti-actions { display: flex; gap: 6px; flex: 0 0 auto; }
.empty-list { color: var(--muted); padding: 8px 0 18px; line-height: 1.6; }
.setup-foot {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  flex-wrap: wrap;
  padding-top: 4px;
}

/* player picker (setup) */
.player-picker { position: relative; }
.suggestions {
  position: absolute;
  z-index: 60;
  top: calc(100% + 4px);
  left: 0;
  right: 0;
  max-height: 300px;
  overflow: auto;
  padding: 5px;
  border: 1px solid var(--border);
  border-radius: 12px;
  background: #0b121a;
  box-shadow: var(--shadow);
}
.suggestion {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
  padding: 8px 9px;
  border: 0;
  border-radius: 8px;
  background: transparent;
  color: var(--text);
  text-align: left;
  cursor: pointer;
  font-size: .88rem;
}
.suggestion:hover { background: rgba(255, 255, 255, .07); }
.suggestion.muted { color: var(--muted); cursor: default; }
.suggestion.create { color: var(--blue); }
.suggestion-name { flex: 1; min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.suggestion-meta { color: var(--muted); font-size: .7rem; white-space: nowrap; }
.chip-avatar {
  width: 22px;
  height: 22px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  flex: 0 0 auto;
  font-size: .6rem;
  font-weight: 800;
  color: #fff;
  background: linear-gradient(145deg, hsl(var(--hue) 65% 45%), hsl(var(--hue) 65% 28%));
}
.team-builder {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto minmax(0, 1fr);
  gap: 8px;
  margin: 8px 0 6px;
}
.team-slot {
  display: flex;
  align-items: center;
  gap: 7px;
  min-width: 0;
  min-height: 38px;
  padding: 6px 9px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: rgba(0, 0, 0, .18);
  font-size: .82rem;
}
.team-slot.empty {
  justify-content: center;
  color: var(--muted);
  border-style: dashed;
  background: transparent;
}
.team-slot.filled { background: rgba(62, 166, 255, .08); }
.team-slot-name {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.team-slot button,
.team-remove {
  border: 0;
  background: transparent;
  color: var(--muted);
  cursor: pointer;
  font-size: 1rem;
  line-height: 1;
  padding: 0 2px;
}
.team-slot button:hover,
.team-remove:hover { color: var(--red); }
.team-slot-plus { align-self: center; color: var(--muted); font-weight: 800; }
.team-builder-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  margin-bottom: 8px;
}
.draft-teams {
  display: flex;
  flex-direction: column;
  gap: 7px;
  margin: 4px 0 4px;
  min-height: 28px;
}
.draft-empty { color: var(--muted); font-size: .78rem; }
.draft-team {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 7px 9px;
  border: 1px solid var(--border);
  border-radius: 11px;
  background: rgba(0, 0, 0, .18);
  font-size: .82rem;
}
.draft-team-players {
  flex: 1;
  min-width: 0;
  display: flex;
  align-items: center;
  gap: 7px;
  flex-wrap: wrap;
}
.draft-player { display: inline-flex; align-items: center; gap: 5px; }
.draft-team .amp { color: var(--muted); font-weight: 700; }
.draft-team-meta {
  color: var(--muted);
  font-size: .68rem;
  white-space: nowrap;
}

/* players */
.view-players {
  flex-direction: column;
  gap: 16px;
  padding: clamp(16px, 2vw, 32px);
  overflow: hidden;
}
.players-head {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: 18px;
  flex-wrap: wrap;
}
.players-head h2 { margin: 0; font-size: clamp(1.4rem, 2vw, 2rem); }
.players-head p { margin: 4px 0 0; }
.players-tools { display: flex; gap: 8px; align-items: center; flex-wrap: wrap; }
.players-tools > input {
  width: clamp(180px, 20vw, 280px);
  padding: 10px 12px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #0a1119;
  color: var(--text);
  outline: none;
}
.players-tools > input:focus { border-color: var(--blue); box-shadow: 0 0 0 3px rgba(62, 166, 255, .14); }
.player-create { display: flex; gap: 6px; }
.player-create input {
  width: clamp(140px, 16vw, 210px);
  padding: 10px 12px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #0a1119;
  color: var(--text);
  outline: none;
}
.player-create input:focus { border-color: var(--blue); box-shadow: 0 0 0 3px rgba(62, 166, 255, .14); }
.players-layout {
  flex: 1;
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(0, 1fr) clamp(340px, 30vw, 480px);
  gap: 16px;
}
.player-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
  gap: 12px;
  align-content: start;
  overflow: auto;
  padding-right: 4px;
}
.player-card {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 8px 10px;
  text-align: left;
  padding: 14px;
  border: 1px solid var(--border);
  border-radius: 14px;
  background: linear-gradient(180deg, var(--panel-2), var(--panel));
  color: var(--text);
  cursor: pointer;
  transition: border-color .15s, transform .15s, box-shadow .15s;
}
.player-card:hover { border-color: rgba(62, 166, 255, .5); transform: translateY(-1px); }
.player-card.selected {
  border-color: var(--blue);
  box-shadow: 0 0 0 1px rgba(62, 166, 255, .35), 0 12px 30px rgba(0, 0, 0, .3);
}
.avatar {
  width: 44px;
  height: 44px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  flex: 0 0 auto;
  font-size: 1rem;
  font-weight: 800;
  color: #fff;
  background: linear-gradient(145deg, hsl(var(--hue) 65% 45%), hsl(var(--hue) 65% 28%));
}
.avatar-lg { width: 72px; height: 72px; font-size: 1.5rem; }
.pc-main { min-width: 0; align-self: center; }
.pc-main strong { display: block; font-size: 1.02rem; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.pc-main small { color: var(--muted); font-size: .75rem; }
.pc-stats {
  grid-column: 1 / -1;
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  padding-top: 8px;
  border-top: 1px solid rgba(255, 255, 255, .06);
  color: var(--muted);
  font-size: .78rem;
}
.pc-stats b { color: var(--text); font-variant-numeric: tabular-nums; }
.pc-best { grid-column: 1 / -1; color: var(--gold); font-size: .75rem; }
.player-detail {
  min-height: 0;
  overflow: auto;
  padding: 18px;
  border: 1px solid var(--border);
  border-radius: var(--radius);
  background: linear-gradient(180deg, var(--panel-2), var(--panel));
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, .04);
}
.detail-head { display: flex; align-items: center; gap: 14px; }
.detail-head h3 { margin: 0; font-size: clamp(1.3rem, 1.6vw, 1.8rem); overflow-wrap: anywhere; }
.detail-head .muted { font-size: .8rem; margin-top: 3px; }
.detail-actions { display: flex; gap: 6px; margin: 12px 0 2px; }
.player-stat-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin: 14px 0; }
.stat-tile {
  padding: 10px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: rgba(0, 0, 0, .16);
  min-width: 0;
}
.stat-tile .label {
  color: var(--muted);
  font-size: .62rem;
  letter-spacing: .12em;
  text-transform: uppercase;
  font-weight: 800;
}
.stat-tile .value { margin-top: 3px; font-size: 1.45rem; font-weight: 800; font-variant-numeric: tabular-nums; }
.stat-tile .sub { color: var(--muted); font-size: .68rem; margin-top: 2px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.player-history h4 { margin: 14px 0 8px; font-size: .95rem; }
.player-history table { width: 100%; border-collapse: collapse; }
.player-history th {
  padding: 6px 5px;
  text-align: left;
  color: var(--muted);
  font-size: .62rem;
  letter-spacing: .1em;
  text-transform: uppercase;
  border-bottom: 1px solid var(--border);
  white-space: nowrap;
}
.player-history td { padding: 7px 5px; border-bottom: 1px solid rgba(255, 255, 255, .05); font-size: .78rem; vertical-align: top; }
.player-history td:nth-child(n+3) { text-align: right; font-variant-numeric: tabular-nums; }
.player-history tr:last-child td { border-bottom: none; }
.player-history .small { font-size: .7rem; }
.badge { display: inline-block; padding: 2px 7px; border-radius: 999px; font-weight: 800; font-size: .72rem; }
.badge.done { color: var(--green); background: rgba(55, 178, 77, .12); }
.badge.live { color: var(--blue); background: rgba(62, 166, 255, .12); }
.player-empty { color: var(--muted); padding: 18px 4px; line-height: 1.6; }

@media (max-width: 1000px) {
  .view-players { overflow: auto; }
  .players-layout { grid-template-columns: 1fr; }
  .player-grid { overflow: visible; }
  .player-detail { overflow: visible; }
}

@media (max-width: 620px) {
  .team-builder { grid-template-columns: 1fr; }
  .team-slot-plus { display: none; }
  .team-builder-bar { align-items: flex-start; }
  .draft-team { align-items: flex-start; }
  .players-head { align-items: flex-start; }
  .players-tools { width: 100%; }
  .players-tools > input { width: 100%; }
  .player-create { width: 100%; }
  .player-create input { flex: 1; width: auto; }
  .player-grid { grid-template-columns: 1fr; }
  .player-stat-grid { grid-template-columns: repeat(2, 1fr); }
}

/* empty tournament */
.empty-state {
  min-height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 40px;
  text-align: center;
}
.empty-icon { font-size: 3rem; }
.empty-state h3 { margin: 6px 0 0; }
.empty-state p { margin: 0 0 12px; color: var(--muted); }

/* toast */
.toast {
  position: fixed;
  left: 50%;
  bottom: 28px;
  transform: translate(-50%, 20px);
  z-index: 100;
  padding: 11px 18px;
  border: 1px solid var(--border);
  border-radius: 999px;
  background: rgba(15, 22, 32, .96);
  box-shadow: var(--shadow);
  color: var(--text);
  font-size: .9rem;
  opacity: 0;
  pointer-events: none;
  transition: opacity .2s, transform .2s;
}
.toast.show { opacity: 1; transform: translate(-50%, 0); }
.toast.error { border-color: rgba(255, 92, 92, .45); color: #ffc9c9; }

/* scrollbars */
* { scrollbar-width: thin; scrollbar-color: #2b3b4e transparent; }
*::-webkit-scrollbar { width: 9px; height: 9px; }
*::-webkit-scrollbar-thumb { background: #2b3b4e; border-radius: 8px; }
*::-webkit-scrollbar-track { background: transparent; }

/* ── TV mode ───────────────────────────────────────────────────────────── */

body.tv .operator-only,
body.tv .operator-controls,
body.tv #btn-save-match,
body.tv .nav-btn[data-view="setup"] { display: none !important; }

body.tv .topbar { padding-top: 8px; padding-bottom: 8px; background: rgba(7, 11, 16, .9); }
body.tv .brand-sub { display: none; }
body.tv .view-tournament { padding: 8px 14px 14px; gap: 8px; }
body.tv .bracket-wrap { border-color: transparent; background: transparent; box-shadow: none; }
body.tv .bracket { padding: 4px 2px; }
body.tv .match,
body.tv .next-item { cursor: default; pointer-events: none; }
body.tv .match:hover { transform: none; border-color: var(--border); box-shadow: none; }
body.tv .next-item:hover { transform: none; border-color: rgba(255, 255, 255, .06); }
body.tv .match.live:hover { border-color: rgba(62, 166, 255, .85); }
body.tv .sidebar { width: 100%; }
body.tv .next-list { max-height: 34vh; overflow: auto; }
body.tv .nav-sep { display: none; }

body.tv .view-live { padding: 10px 16px 16px; }
body.tv .team-panel { box-shadow: none; }
body.tv .score { font-size: clamp(7rem, 18vw, 22rem); }

/* ── responsive ────────────────────────────────────────────────────────── */

@media (max-width: 1180px) {
  .live-stage { grid-template-columns: minmax(0, 1fr) minmax(150px, .45fr) minmax(0, 1fr); }
  .tournament-body { grid-template-columns: minmax(0, 1fr) clamp(250px, 28vw, 340px); }
  .brand-sub { max-width: 26vw; }
}

@media (max-width: 620px) {
  .import-row { flex-wrap: wrap; }
  .import-row input[type="number"] { flex: 1 1 100%; }
  .import-row .btn { flex: 1 1 100%; }
}

@media (max-width: 900px) {
  body { overflow: auto; }
  .app { height: auto; min-height: 100dvh; }
  .topbar { flex-wrap: wrap; gap: 10px; padding: 10px 12px; }
  .brand { order: 1; }
  .brand-sub { max-width: 52vw; }
  .live-mini { order: 2; margin-left: auto; }
  .nav { order: 3; width: 100%; overflow-x: auto; padding-bottom: 2px; }
  .nav-btn, .icon-btn { padding: 8px 11px; }
  .conn { margin-left: auto; }

  .main { min-height: calc(100vh - 120px); min-height: calc(100dvh - 120px); }
  .view-live { overflow: auto; padding: 12px; }
  .live-stage { grid-template-columns: 1fr; grid-template-rows: auto auto auto; gap: 12px; }
  .team-panel { min-height: 220px; padding: 18px; }
  .score { font-size: clamp(5rem, 26vw, 10rem); }
  .live-center { order: 2; }
  .view-tournament { overflow: auto; padding: 12px; }
  .tournament-body { grid-template-columns: 1fr; }
  .bracket-wrap { min-height: 58vh; }
  .sidebar { flex-direction: column; overflow: visible; }
  .setup-grid { grid-template-columns: 1fr; }
  .tournament-head { align-items: flex-start; flex-direction: column; }
  .setup-head { align-items: flex-start; }
}

@media (max-height: 720px) and (min-width: 901px) {
  .view-live { padding: 10px 18px; }
  .score { font-size: clamp(5rem, 14vw, 13rem); }
  .team-panel { padding: 14px; }
  .vs { font-size: clamp(1.8rem, 3.5vw, 3.5rem); }
  .hint { display: none; }
  .view-tournament { padding: 8px 14px; }
  .match { padding: 6px 9px; }
  .team-row { padding: 3px 0; }
}

@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after { animation-duration: .001ms !important; animation-iteration-count: 1 !important; transition-duration: .001ms !important; }
}

button:disabled { opacity: .55; cursor: not-allowed; transform: none !important; }

:focus-visible { outline: 2px solid var(--blue); outline-offset: 2px; }

</style>
</head>
<body>
  <div class="app">
    <header class="topbar">
      <div class="brand">
        <div class="brand-ball">⚽</div>
        <div class="brand-text">
          <h1>TOURNEY<span>MASTER</span></h1>
          <div class="brand-sub" id="brand-sub">Casual Match</div>
        </div>
      </div>

      <div class="live-mini" id="live-mini">
        <div class="mini-side blue">
          <span class="mini-name" id="mini-t1">Blau</span>
          <span class="mini-score" id="mini-s1">0</span>
        </div>
        <span class="mini-colon">:</span>
        <div class="mini-side red">
          <span class="mini-score" id="mini-s2">0</span>
          <span class="mini-name" id="mini-t2">Rot</span>
        </div>
      </div>

      <nav class="nav">
        <button class="nav-btn active" data-view="live">Live</button>
        <button class="nav-btn" data-view="tournament">Turnier</button>
        <button class="nav-btn" data-view="players">Spieler</button>
        <button class="nav-btn" data-view="setup">Setup</button>
        <span class="nav-sep"></span>
        <button class="icon-btn" id="btn-tv" title="TV-Modus (T)">TV</button>
        <button class="icon-btn" id="btn-fullscreen" title="Vollbild (F)">⛶</button>
        <span class="conn connecting" id="conn" title="Verbindung">●</span>
      </nav>
    </header>

    <main class="main">
      <!-- ───────────────────────── LIVE ───────────────────────── -->
      <section class="view view-live active" id="view-live">
        <div class="live-stage">
          <article class="team-panel blue" id="panel-1">
            <div class="team-kicker">BLAU</div>
            <h2 class="team-title" id="live-name1">Team Blue</h2>
            <div class="score" id="score1">0</div>
            <div class="operator-controls">
              <button class="score-btn sub" data-cmd="t1_sub" aria-label="Blau minus">−1</button>
              <button class="score-btn add" data-cmd="t1_add" aria-label="Blau plus">+1</button>
            </div>
          </article>

          <div class="live-center">
            <div class="match-status" id="match-status">CASUAL MATCH</div>
            <div class="vs">VS</div>
            <div class="center-actions">
              <button class="btn primary hidden" id="btn-save-match">Spiel speichern &amp; weiter</button>
              <button class="btn warn operator-only" data-cmd="reset">Reset Scoreboard</button>
              <button class="btn ghost operator-only" id="btn-open-tournament">Turnierübersicht</button>
            </div>
            <div class="hint operator-only">
              <kbd>1</kbd>/<kbd>2</kbd> Tor · <kbd>R</kbd> Reset · <kbd>T</kbd> TV
            </div>
          </div>

          <article class="team-panel red" id="panel-2">
            <div class="team-kicker">ROT</div>
            <h2 class="team-title" id="live-name2">Team Red</h2>
            <div class="score" id="score2">0</div>
            <div class="operator-controls">
              <button class="score-btn sub" data-cmd="t2_sub" aria-label="Rot minus">−1</button>
              <button class="score-btn add" data-cmd="t2_add" aria-label="Rot plus">+1</button>
            </div>
          </article>
        </div>
        <div class="goal-overlay" id="goal-overlay">TOR!</div>
      </section>

      <!-- ────────────────────── TOURNAMENT ─────────────────────── -->
      <section class="view view-tournament" id="view-tournament">
        <div class="tournament-head">
          <div class="tournament-heading">
            <h2 id="t-title">Kein Turnier</h2>
            <div class="t-meta" id="t-meta">Erstelle ein Turnier im Setup.</div>
            <div class="champion hidden" id="champion"></div>
          </div>
          <div class="tournament-actions">
            <button class="btn ghost operator-only hidden" id="btn-shuffle-schedule">Spielplan mischen</button>
            <button class="btn ghost" id="btn-back-live" data-view="live">Live</button>
            <button class="btn ghost operator-only" id="btn-edit-tournament" data-view="setup">Setup</button>
          </div>
        </div>

        <div class="tournament-body">
          <div class="bracket-wrap" id="bracket-wrap">
            <div class="bracket" id="bracket"></div>
            <div class="standings hidden" id="standings"></div>
            <div class="empty-state hidden" id="empty-tournament">
              <div class="empty-icon">🏆</div>
              <h3>Noch kein Turnier</h3>
              <p>Lege im Setup ein Turnier an oder lade das Demo-Turnier.</p>
              <button class="btn primary" data-view="setup">Zum Setup</button>
            </div>
          </div>

          <aside class="sidebar">
            <div class="side-card live-card" id="live-card"></div>
            <div class="side-card">
              <div class="side-title">Nächste Spiele</div>
              <div class="next-list" id="next-matches"></div>
            </div>
          </aside>
        </div>
      </section>

      <!-- ───────────────────────── SPIELER ─────────────────────── -->
      <section class="view view-players" id="view-players">
        <div class="players-head">
          <div>
            <h2>Spieler</h2>
            <p class="muted">Profile, Platzierungen und Statistiken.</p>
          </div>
          <div class="players-tools">
            <input type="search" id="player-filter" placeholder="Spieler filtern…" autocomplete="off">
            <div class="player-create operator-only">
              <input type="text" id="new-player-name" placeholder="Neuer Spieler…" autocomplete="off">
              <button class="btn primary" id="btn-create-player" type="button">Anlegen</button>
            </div>
          </div>
        </div>
        <div class="players-layout">
          <div id="player-list" class="player-grid"></div>
          <aside id="player-detail" class="player-detail"></aside>
        </div>
      </section>

      <!-- ───────────────────────── SETUP ───────────────────────── -->
      <section class="view view-setup" id="view-setup">
        <div class="setup-head">
          <div>
            <h2>Setup</h2>
            <p class="muted">Turniere verwalten und neue erstellen.</p>
          </div>
          <div class="setup-actions">
            <button class="btn ghost" id="btn-setup-demo">Demo-Turnier laden</button>
            <button class="btn primary" id="btn-new-tournament">+ Neues Turnier</button>
          </div>
        </div>

        <div class="setup-grid">
          <div class="card">
            <h3>Turniere</h3>
            <div id="tournament-list" class="tournament-list"></div>
          </div>

          <div class="card" id="create-card">
            <h3>Neues Turnier</h3>
            <label for="t-name">Name</label>
            <input type="text" id="t-name" placeholder="z. B. Friday Night Foosball Cup" autocomplete="off">

            <label for="t-mode">Modus</label>
            <select id="t-mode">
              <option value="elimination">K.-o.-System (Bracket)</option>
              <option value="seeding">Jeder gegen jeden (Tabelle)</option>
            </select>

            <label for="player-search">Spieler suchen</label>
            <div class="player-picker">
              <input type="text" id="player-search" placeholder="Spieler suchen oder neu anlegen…" autocomplete="off">
              <div id="player-suggestions" class="suggestions hidden"></div>
            </div>
            <div id="team-builder" class="team-builder"></div>
            <div class="team-builder-bar">
              <button class="btn ghost small" id="btn-add-team" type="button" disabled>+ Team hinzufügen</button>
              <span class="fine">1–2 Spieler pro Team</span>
            </div>
            <div id="draft-teams" class="draft-teams"></div>
            <p class="fine">Für 2 gegen 2 zwei Spieler pro Team wählen. Teams brauchen keinen Namen; alle Teams sollten gleich groß sein.</p>

            <div class="import-box">
              <div class="import-title">Teams übernehmen</div>
              <label for="t-source">Quellturnier</label>
              <select id="t-source">
                <option value="">— Turnier wählen —</option>
              </select>
              <div class="import-row">
                <select id="t-source-mode" aria-label="Auswahlmodus">
                  <option value="all">Alle Teams</option>
                  <option value="top">Top N</option>
                </select>
                <input type="number" id="t-source-count" class="hidden" min="1" value="4" inputmode="numeric" aria-label="Anzahl der Top-Teams">
                <button class="btn ghost" id="btn-import-teams" type="button">Übernehmen</button>
              </div>
              <p class="fine" id="import-hint">Wähle ein Quellturnier.</p>
            </div>

            <button class="btn primary big" id="btn-generate">Turnier erstellen</button>
          </div>
        </div>

        <div class="setup-foot">
          <div id="connection-note" class="fine">Verbinde mit dem ESP32 oder starte den lokalen Server.</div>
          <button class="btn ghost small" id="btn-mock">Demo-Modus (ohne ESP32)</button>
        </div>
      </section>
    </main>
  </div>

  <div class="toast" id="toast"></div>
  <script>
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

</script>
</body>
</html>

)rawliteral";
