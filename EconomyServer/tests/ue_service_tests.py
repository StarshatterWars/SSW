"""Actual Unreal snapshot through a separate native HTTP process; isolated state only."""
import json, pathlib, socket, subprocess, sys, tempfile, time, urllib.request
exe, data, fixture = sys.argv[1:4]
payload = json.loads(pathlib.Path(fixture).read_text(encoding='utf-8-sig'))
checks = 0
def check(value, reason):
    global checks
    if not value: raise AssertionError(reason)
    checks += 1
with tempfile.TemporaryDirectory(prefix='SSW UE cross process ') as folder:
    root=pathlib.Path(folder)
    with socket.socket() as sock:
        sock.bind(('127.0.0.1',0)); port=sock.getsockname()[1]
    command=[exe,'--data',data,'--state',str(root/'state'),'--port',str(port),'--await-bootstrap']
    def call(path, body=None, key=None):
        req=urllib.request.Request(f'http://127.0.0.1:{port}{path}',data=None if body is None else json.dumps(body).encode(),headers={'Content-Type':'application/json','X-SSW-Authority':key or ''})
        with urllib.request.urlopen(req,timeout=10) as reply: return json.load(reply)
    def start(log):
        process=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT)
        for _ in range(400):
            if process.poll() is not None: raise AssertionError('Native service exited')
            try: return process,call('/status')
            except OSError: time.sleep(.05)
        process.terminate(); process.wait(); raise AssertionError('Native service startup timed out')
    with open(root/'service.log','wb') as log:
        process,status=start(log)
        try:
            check(not status['npcAuthorityActive'],'Standby has no independent economy')
            key=(root/'state/authority.key').read_text()
            request={'action':'bootstrap_npc','format':'SSW-UE-NPC-1','sourceId':'actual-UE-test','handoffId':'cross-process-fixture','snapshot':payload}
            accepted=call('/v1/commands',request,key)
            check(accepted['ok'],'Actual Unreal snapshot accepted over HTTP')
            check(call('/v1/commands',request,key)==accepted,'Lost activation reply retry is idempotent')
            projection=call('/v1/commands',{'action':'npc_projection','worldId':status['worldId']},key)
            check('Pilot' not in projection['economy']['wallets'],'Pilot money remains outside server')
            check('Pilot/Cargo' not in projection['economy']['inventories'],'Pilot cargo remains outside server')
            check('9007199254740993' in projection['economy']['wallets'].values(),'Exact int64 budget survives HTTP')
            check(len(projection['traffic']['journeys'])==1,'Active loaded freight survives handoff')
            check(len(projection['economy']['jobs'])==1,'Active production survives handoff')
            call('/shutdown',{},key); check(process.wait(timeout=20)==0,'Durable graceful shutdown')
            process,restarted=start(log); key=(root/'state/authority.key').read_text()
            check(restarted['npcAuthorityActive'] and restarted['worldId']==status['worldId'],'Restart resumes same world')
            check(call('/v1/commands',request,key)==accepted,'Restart recovers same activation receipt')
            projection=call('/v1/commands',{'action':'npc_projection','worldId':status['worldId']},key)
            check(len(projection['traffic']['journeys'])==1,'Restart retains shipment without duplicating it')
            call('/shutdown',{},key); check(process.wait(timeout=20)==0,'Recovered service checkpoints cleanly')
        finally:
            if process.poll() is None: process.terminate(); process.wait(timeout=20)
print(f'{checks} actual UE HTTP cross-process checks passed')
