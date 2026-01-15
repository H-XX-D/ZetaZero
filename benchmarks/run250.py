#!/usr/bin/env python3
import json,time,requests,subprocess
def p():
    try: return float(subprocess.run(['nvidia-smi','--query-gpu=power.draw','--format=csv,noheader,nounits'],capture_output=True,text=True).stdout.strip())
    except: return 0
def t(u,q,h):
    m=h[-10:]+[{'role':'user','content':q}]
    p0,t0=p(),time.time()
    r=requests.post(f'{u}/v1/chat/completions',json={'model':'llama','messages':m,'max_tokens':150},timeout=120)
    lat=time.time()-t0
    e=(p0+p())/2*lat
    if r.ok:
        d=r.json()
        return {'ok':1,'lat':lat,'tok':d.get('usage',{}).get('total_tokens',0),'e':e,'r':d['choices'][0]['message']['content'][:60]}
    return {'ok':0,'lat':lat,'e':e}
Q=['Backprop','REST vs GraphQL','GC','CAP','Concurrency','TLS','Microservices','Indexes','Eventual consistency','Bloom filter']*25
def b(n,u):
    print(f'=== {n} ===')
    res,h,E=[],[],0
    for i,q in enumerate(Q):
        r=t(u,q,h)
        res.append(r)
        if r['ok']:E+=r['e'];h+=[{'role':'user','content':q},{'role':'assistant','content':r.get('r','')}]
        print(f'[{i+1:03d}] {r["lat"]:.1f}s {r["e"]:.0f}J')
    ok=[x for x in res if x['ok']]
    return {'n':n,'ok':len(ok),'time':sum(x['lat'] for x in ok),'E':E}
z=b('Zeta','http://localhost:8080')
bl=b('Base','http://localhost:9000')
print(f'\nZeta: {z["ok"]}/250, {z["time"]:.0f}s, {z["E"]:.0f}J')
print(f'Base: {bl["ok"]}/250, {bl["time"]:.0f}s, {bl["E"]:.0f}J')
if bl['E']>0: print(f'Savings: {(bl["E"]-z["E"])/bl["E"]*100:.1f}%')
json.dump({'z':z,'b':bl},open('/runpod-volume/250_results.json','w'))
print('Done!')
