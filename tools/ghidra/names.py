import os,sys
import pyghidra; pyghidra.start()
from ghidra.base.project import GhidraProject
P0=os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.chdir(P0)
proj=GhidraProject.openProject(os.path.join(P0,"ghidra"),"panzers",True)
prog=proj.openProgram("/","PANZERS.exe",True)
fm=prog.getFunctionManager(); st=prog.getSymbolTable()
n=0
for f in fm.getFunctions(True):
    nm=f.getName(True)
    if not nm.startswith('FUN_') and '::' in nm and not nm.startswith(('Catch','Unwind')):
        n+=1
        if 'SSuperWindow' in nm or 'SGepard' in nm or 'SFileSystem' in nm or 'SDXWindow' in nm: print(f.getEntryPoint(), nm)
print('named class funcs',n)
for a in sys.argv[1:]:
    f=fm.getFunctionAt(prog.getAddressFactory().getAddress(a)); print(a, f.getName(True) if f else None)
