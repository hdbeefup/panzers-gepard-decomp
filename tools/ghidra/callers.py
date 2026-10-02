import os,sys
import pyghidra; pyghidra.start()
from ghidra.base.project import GhidraProject
P0=os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.chdir(P0)
proj=GhidraProject.openProject(os.path.join(P0,"ghidra"),"panzers",True)
prog=proj.openProgram("/","PANZERS.exe",True)
fm=prog.getFunctionManager(); rm=prog.getReferenceManager(); af=prog.getAddressFactory()
out=open(os.path.join(P0,'callers.txt'),'a')
def name(a):
    f=fm.getFunctionContaining(a); return '%s@%s'%(f.getName(True),f.getEntryPoint()) if f else 'nofunc@%s'%a
for t in sys.argv[1:]:
    if t.startswith('n:'):
        for f in fm.getFunctions(True):
            if t[2:] in f.getName(True): out.write('NAME %s %s\n'%(f.getEntryPoint(),f.getName(True)))
        continue
    a=af.getAddress(t); depth=0
    out.write('== %s (%s)\n'%(t,name(a)))
    for r in rm.getReferencesTo(a):
        out.write('   <- %s %s from %s\n'%(r.getReferenceType(),r.getFromAddress(),name(r.getFromAddress())))
    f=fm.getFunctionAt(a)
    if f:
        for c in f.getCalledFunctions(None): out.write('   -> %s@%s\n'%(c.getName(True),c.getEntryPoint()))
out.close()
