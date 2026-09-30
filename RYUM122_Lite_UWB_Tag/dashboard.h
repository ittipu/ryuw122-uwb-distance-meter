// Dashboard HTML served at / by the tag.
// Kept out of the .ino so the Arduino prototype generator does not
// mistake the JavaScript "function ..." lines for C++ functions.
#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>UWB Meter</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500&family=JetBrains+Mono:wght@400;500&display=swap">
<style>
  :root{--bg:#0A0E14;--panel:#131820;--border:#232B3B;--text:#E8ECF1;--dim:#8B95A7;--mute:#5B6577;--accent:#22D3EE;--good:#10B981;--bad:#F87171;--mono:'JetBrains Mono',monospace;}
  html,body{margin:0;height:100%;background:var(--bg);color:var(--text);font-family:Inter,sans-serif;-webkit-font-smoothing:antialiased;}
  .wrap{max-width:720px;margin:0 auto;padding:24px 16px;display:flex;flex-direction:column;gap:16px;box-sizing:border-box;}
  header{display:flex;justify-content:space-between;align-items:center;}
  .mark{width:32px;height:32px;border-radius:8px;background:linear-gradient(135deg,var(--accent),#0E7490);display:grid;place-items:center;color:#041519;font-family:var(--mono);font-weight:500;font-size:12px;}
  .brand{display:flex;align-items:center;gap:10px;}
  .brand .t{font-family:var(--mono);font-size:12px;letter-spacing:.14em;text-transform:uppercase;color:var(--dim);}
  .pill{padding:6px 10px;background:var(--panel);border:1px solid var(--border);border-radius:6px;font-family:var(--mono);font-size:11px;letter-spacing:.08em;color:var(--dim);}
  .pill.live{color:var(--accent);border-color:rgba(34,211,238,.4);}
  .pill.off{color:var(--mute);}
  .pill .dot{display:inline-block;width:6px;height:6px;border-radius:50%;background:currentColor;margin-right:6px;vertical-align:1px;animation:blink 1.4s ease-in-out infinite;}
  @keyframes blink{50%{opacity:.3;}}
  .hero{background:var(--panel);border:1px solid var(--border);border-radius:16px;padding:24px 16px 16px;display:flex;flex-direction:column;align-items:center;transition:border-color .2s;}
  .hero.good{border-color:rgba(16,185,129,.5);}
  .hero.bad{border-color:rgba(248,113,113,.55);}
  .lbl{font-family:var(--mono);font-size:11px;letter-spacing:.22em;color:var(--mute);text-transform:uppercase;margin-bottom:6px;}
  .dist{font-family:var(--mono);font-weight:500;font-size:clamp(72px,18vw,148px);line-height:1;letter-spacing:-.03em;font-variant-numeric:tabular-nums;display:flex;align-items:baseline;gap:10px;transition:color .16s;}
  .hero.good .dist{color:var(--good);} .hero.bad .dist{color:var(--bad);}
  .dist .u{font-size:clamp(20px,4vw,28px);color:var(--mute);}
  .hero.good .dist .u,.hero.bad .dist .u{color:currentColor;opacity:.55;}
  .spark{width:100%;height:80px;margin-top:16px;}
  .stats{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;}
  .stat{background:var(--panel);border:1px solid var(--border);border-radius:12px;padding:12px 14px;}
  .stat .k{font-family:var(--mono);font-size:10px;letter-spacing:.18em;text-transform:uppercase;color:var(--mute);}
  .stat .v{font-family:var(--mono);font-variant-numeric:tabular-nums;font-size:20px;margin-top:4px;}
  .stat .v .u{color:var(--mute);font-size:11px;margin-left:2px;}
  .ctrl{background:var(--panel);border:1px solid var(--border);border-radius:12px;padding:14px 16px;display:flex;flex-direction:column;gap:10px;}
  .ctrl .row{display:flex;justify-content:space-between;align-items:center;font-family:var(--mono);font-size:11px;letter-spacing:.14em;text-transform:uppercase;color:var(--mute);}
  .ctrl .row .val{color:var(--text);text-transform:none;letter-spacing:0;font-size:14px;}
  input[type=range]{-webkit-appearance:none;width:100%;height:4px;background:#1A2130;border-radius:2px;outline:none;}
  input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:var(--accent);border:2px solid var(--panel);}
  .seg{display:inline-flex;background:#1A2130;border:1px solid var(--border);border-radius:8px;padding:2px;gap:2px;}
  .seg button{background:none;border:none;color:var(--dim);padding:6px 10px;font-family:var(--mono);font-size:11px;letter-spacing:.1em;text-transform:uppercase;border-radius:6px;cursor:pointer;}
  .seg button.on{background:var(--panel);color:var(--accent);}
  @media(max-width:520px){.stats{grid-template-columns:repeat(2,1fr);}}
</style></head>
<body><div class="wrap">
  <header>
    <div class="brand"><div class="mark">UWB</div><span class="t">Distance Meter</span></div>
    <span class="pill" id="pill"><span class="dot"></span>CONNECTING</span>
  </header>
  <div class="hero" id="hero">
    <div class="lbl">T1 ↔ A1 · Channel 5</div>
    <div class="dist"><span id="d">0.00</span><span class="u">m</span></div>
    <svg class="spark" id="spark" viewBox="0 0 400 80" preserveAspectRatio="none">
      <defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#22D3EE" stop-opacity=".28"/><stop offset="1" stop-color="#22D3EE" stop-opacity="0"/></linearGradient></defs>
      <path id="sf" fill="url(#g)"/><path id="sl" fill="none" stroke="#22D3EE" stroke-width="1.4"/>
    </svg>
  </div>
  <div class="stats">
    <div class="stat"><div class="k">Min</div><div class="v"><span id="mn">0.00</span><span class="u">m</span></div></div>
    <div class="stat"><div class="k">Max</div><div class="v"><span id="mx">0.00</span><span class="u">m</span></div></div>
    <div class="stat"><div class="k">Mean</div><div class="v"><span id="mean">0.00</span><span class="u">m</span></div></div>
    <div class="stat"><div class="k">σ</div><div class="v"><span id="std">0.00</span><span class="u">m</span></div></div>
  </div>
  <div class="ctrl">
    <div class="row"><span>Threshold</span><span><span class="val" id="tv">2.00</span> m</span></div>
    <input type="range" id="th" min="0.2" max="10" step="0.05" value="2">
    <div class="row" style="margin-top:6px;"><span>Alarm mode</span>
      <span class="seg" id="mode"><button data-m="off" class="on">Off</button><button data-m="near">&lt; th</button><button data-m="far">&gt; th</button></span>
    </div>
  </div>
</div>
<script>
const buf=[],HIST=10000;let mode='off';
const d=document.getElementById('d'),hero=document.getElementById('hero'),pill=document.getElementById('pill');
const mn=document.getElementById('mn'),mx=document.getElementById('mx'),mean=document.getElementById('mean'),std=document.getElementById('std');
const th=document.getElementById('th'),tv=document.getElementById('tv');
const sl=document.getElementById('sl'),sf=document.getElementById('sf');
th.oninput=()=>{tv.textContent=parseFloat(th.value).toFixed(2);alarm();};
document.querySelectorAll('#mode button').forEach(b=>b.onclick=()=>{document.querySelectorAll('#mode button').forEach(x=>x.classList.remove('on'));b.classList.add('on');mode=b.dataset.m;alarm();});
function stats(){if(!buf.length)return null;let a=1/0,b=-1/0,s=0;for(const x of buf){if(x.d<a)a=x.d;if(x.d>b)b=x.d;s+=x.d;}const m=s/buf.length;let v=0;for(const x of buf)v+=(x.d-m)**2;return{mn:a,mx:b,mean:m,std:Math.sqrt(v/buf.length)};}
function draw(){if(buf.length<2){sl.setAttribute('d','');sf.setAttribute('d','');return;}
  const s=stats();let lo=s.mn-.08,hi=s.mx+.08;if(hi-lo<.25){const c=(lo+hi)/2;lo=c-.15;hi=c+.15;}
  const W=400,H=80,now=performance.now(),t0=now-HIST;let p='';const xs=[],ys=[];
  for(const x of buf){const px=(x.t-t0)/HIST*W,py=H-(x.d-lo)/(hi-lo)*H;xs.push(px);ys.push(py);}
  p='M '+xs[0].toFixed(1)+' '+ys[0].toFixed(1);for(let i=1;i<xs.length;i++)p+=' L '+xs[i].toFixed(1)+' '+ys[i].toFixed(1);
  sl.setAttribute('d',p);sf.setAttribute('d',p+' L '+xs.at(-1).toFixed(1)+' '+H+' L '+xs[0].toFixed(1)+' '+H+' Z');
  mn.textContent=s.mn.toFixed(2);mx.textContent=s.mx.toFixed(2);mean.textContent=s.mean.toFixed(2);std.textContent=s.std.toFixed(3);
}
function alarm(){hero.classList.remove('good','bad');if(!buf.length||mode==='off')return;const v=buf.at(-1).d,t=parseFloat(th.value);const trig=(mode==='near'&&v<t)||(mode==='far'&&v>t);hero.classList.add(trig?'bad':'good');}
function push(v){const t=performance.now();buf.push({t,d:v});const c=t-HIST;while(buf.length&&buf[0].t<c)buf.shift();d.textContent=v.toFixed(2);draw();alarm();}
function connect(){const s=new WebSocket('ws://'+location.hostname+':81/');
  s.onopen=()=>{pill.className='pill live';pill.innerHTML='<span class="dot"></span>LIVE';};
  s.onclose=()=>{pill.className='pill off';pill.innerHTML='<span class="dot"></span>OFFLINE';setTimeout(connect,1200);};
  s.onmessage=e=>{try{const j=JSON.parse(e.data);if(typeof j.d==='number')push(j.d);}catch(_){}};
}
connect();setInterval(draw,80);
</script></body></html>
)HTML";
