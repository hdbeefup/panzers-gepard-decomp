import os,re,csv,sys
os.environ.setdefault("GHIDRA_INSTALL_DIR", r"C:\Users\swine\scoop\apps\ghidra\current")
import pyghidra; pyghidra.start()
from ghidra.base.project import GhidraProject
P0=os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.chdir(P0); sys.path.insert(0,os.path.join(P0,"scripts"))
proj=GhidraProject.openProject(os.path.join(P0,"ghidra"),"panzers",True)
prog=proj.openProgram("/","PANZERS.exe",True)
af=prog.getAddressFactory().getDefaultAddressSpace(); fm=prog.getFunctionManager(); rm=prog.getReferenceManager()
pat=re.compile(r'(?<![A-Za-z0-9_])([A-Z][A-Za-z0-9_]*)::(~?[A-Za-z_][A-Za-z0-9_]*|operator\s*\S+)')
rows=[];rtti=[];misses=0
for line in open('strings.txt',encoding='latin1'):
    va,s=line.rstrip('\n').split(' ',1); v=int(va,16)
    if v<0x7ea000 or v>=0x962000: continue
    if s.startswith('.?AV') or s.startswith('.?AU'): rtti.append((v,s)); continue
    ms=pat.findall(s)
    if not ms: continue
    if 'RakNet::' in s and not s.startswith('RakNet'): pass
    name='%s::%s'%ms[0]
    found=False
    # strings may be referenced at their start or a few bytes in (leading chars stripped by strings dump)
    for d in range(0,4):
        for r in rm.getReferencesTo(af.getAddress(v-d)):
            f=fm.getFunctionContaining(r.getFromAddress())
            if f: rows.append(('0x%08x'%f.getEntryPoint().getOffset(),name,s,'0x%08x'%r.getFromAddress().getOffset())); found=True
        if found: break
    if not found: misses+=1; rows.append(('',name,s,''))
with open('panzers_symbols.csv','w',newline='',encoding='utf-8') as fo:
    w=csv.writer(fo); w.writerow(['address','name','evidence_string'])
    seen=set()
    for a,n,s,frm in sorted(rows):
        if (a,n,s) in seen: continue
        seen.add((a,n,s)); w.writerow([a,n,s])
with open('rtti.txt','w') as fo:
    for v,s in rtti: fo.write('%08x %s\n'%(v,s))
print('rows',len(seen),'unref',misses,'rtti',len(rtti),'funcs',fm.getFunctionCount())
