import os,sys
os.environ.setdefault("GHIDRA_INSTALL_DIR", r"C:\Users\swine\scoop\apps\ghidra\current")
import pyghidra; pyghidra.start()
from ghidra.base.project import GhidraProject
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor
P0=os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.chdir(P0); sys.path.insert(0,os.path.join(P0,"scripts"))
proj=GhidraProject.openProject(os.path.join(P0,"ghidra"),"panzers",True)
prog=proj.openProgram("/","PANZERS.exe",True)
af=prog.getAddressFactory().getDefaultAddressSpace(); fm=prog.getFunctionManager(); rm=prog.getReferenceManager()
dec=DecompInterface(); dec.openProgram(prog); mon=ConsoleTaskMonitor()
data=open('PANZERS.exe','rb').read()
out=open(sys.argv[1],'w',encoding='utf-8')
targets=[]
for t in sys.argv[2:]:
    if t.startswith('s:'):
        s=t[2:].encode(); i=0
        import pefile
        from pe import find
        for v in find(s):
            for d in range(0,8):
                for r in rm.getReferencesTo(af.getAddress(v-d)):
                    f=fm.getFunctionContaining(r.getFromAddress())
                    if f: out.write('// string %r @%x ref from %s in %s\n'%(t[2:],v,r.getFromAddress(),f.getName())); targets.append(f)
    else:
        f=fm.getFunctionContaining(af.getAddress(int(t,16)))
        if f: targets.append(f)
seen=set()
for f in targets:
    if f.getEntryPoint() in seen: continue
    seen.add(f.getEntryPoint())
    r=dec.decompileFunction(f,120,mon)
    out.write('\n// ===== %s @ %s\n'%(f.getName(),f.getEntryPoint()))
    out.write(r.getDecompiledFunction().getC() if r.decompileCompleted() else '// FAILED\n')
# extra: report function info
for f in targets:
    out.write('// FUNC %s %s size=%d\n'%(f.getName(),f.getEntryPoint(),f.getBody().getNumAddresses()))
