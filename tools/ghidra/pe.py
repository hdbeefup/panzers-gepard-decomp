import pefile
pe=pefile.PE('PANZERS.exe'); data=open('PANZERS.exe','rb').read()
def off2va(o): return pe.get_rva_from_offset(o)+0x400000
def va2off(v): return pe.get_offset_from_rva(v-0x400000)
def find(s):
    r=[];i=data.find(s)
    while i>=0: r.append(off2va(i)); i=data.find(s,i+1)
    return r
def sec(v):
    for s in pe.sections:
        if s.VirtualAddress+0x400000<=v<s.VirtualAddress+0x400000+s.Misc_VirtualSize: return s.Name.rstrip(b'\0').decode()
