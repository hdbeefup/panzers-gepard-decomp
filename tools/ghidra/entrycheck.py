import os
import pyghidra; pyghidra.start()
from ghidra.base.project import GhidraProject
P0=os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.chdir(P0)
proj=GhidraProject.openProject(os.path.join(P0,"ghidra"),"panzers",True)
prog=proj.openProgram("/","PANZERS.exe",True)
fm=prog.getFunctionManager(); af=prog.getAddressFactory()
L=[l.strip() for l in open('export_funs.txt')]
hit=[a for a in L if fm.getFunctionAt(af.getAddress('0x'+a))]
print('export FUN_ addrs',len(L),'that are HD function entries',len(hit))
mem=prog.getMemory()
for a in ['00401010','00449b40','0053bcf3','0068df54','00790910']:
    f=fm.getFunctionContaining(af.getAddress('0x'+a)); print(a, 'HD func containing:', f.getName() if f else None, 'entry' if fm.getFunctionAt(af.getAddress('0x'+a)) else 'not-entry')
