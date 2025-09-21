import React, {useEffect, useState} from 'react';

const ENDPOINTS = [
  {name: 'health', path: '/health'},
  {name: 'api', path: '/api/data'},
];

function percentile(arr, p){
  if(arr.length===0) return null;
  const sorted = arr.slice().sort((a,b)=>a-b);
  const idx = Math.ceil((p/100)*sorted.length)-1;
  return sorted[Math.max(0,Math.min(idx, sorted.length-1))];
}

export default function App(){
  const [results, setResults] = useState({});
  const [running, setRunning] = useState(false);

  async function checkAll(){
    setRunning(true);
    const newRes = {};
    for(const ep of ENDPOINTS){
      const samples = [];
      for(let i=0;i<50;i++){
        const t1 = performance.now();
        try{
          const r = await fetch(ep.path, {cache: 'no-store'});
          await r.text();
        }catch(e){ /* ignore */ }
        const t2 = performance.now();
        samples.push(Math.round(t2-t1));
      }
      newRes[ep.name] = {
        p50: percentile(samples,50),
        p95: percentile(samples,95),
        p99: percentile(samples,99),
        latest: samples[samples.length-1]
      }
    }
    setResults(newRes);
    setRunning(false);
  }

  useEffect(()=>{ checkAll(); const id = setInterval(checkAll, 15000); return ()=>clearInterval(id); },[]);

  return (
    <div style={{fontFamily:'Arial, sans-serif', padding:20}}>
      <h2>Verification Dashboard</h2>
      <p>Service port: {window.location.port || "default"}</p>
      <button onClick={checkAll} disabled={running}>{running? 'Running...':'Run Checks'}</button>
      <div style={{display:'flex', gap:20, marginTop:20}}>
        {Object.entries(results).map(([k,v])=> (
          <div key={k} style={{border:'1px solid #ddd', padding:10, borderRadius:8, width:220}}>
            <h4>{k}</h4>
            <p>p50: {v.p50} ms</p>
            <p>p95: {v.p95} ms</p>
            <p>p99: {v.p99} ms</p>
            <p>latest: {v.latest} ms</p>
          </div>
        ))}
      </div>
    </div>
  )
}
