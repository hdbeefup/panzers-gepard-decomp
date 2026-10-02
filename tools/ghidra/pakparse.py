import struct,sys
p=sys.argv[1]; N=int(sys.argv[2]) if len(sys.argv)>2 else 20
f=open(p,'rb'); m,m2,pk,ts=struct.unpack('<4I',f.read(16)); toc=f.read(ts); base=16+ts
print('magic %08x %08x %08x tocsize=%d database=%d'%(m,m2,pk,ts,base))
# sequential scan, track names reconstructed from path stack: prefix refers to the name of the parent on search path.
pos=0;n=0;stack=[] # (endpos_of_subtree?) simpler: keep list of previous names by toc offset
names={}
# recursive tree walk to reconstruct full names
out=[]
def walk(pos,parentname):
    while True:
        e=pos; pre=toc[pos]; ln=toc[pos+1]; suf=toc[pos+2:pos+2+ln].decode('latin1'); pos+=2+ln
        off,size,left,right=struct.unpack_from('<IIBI',toc,pos); pos+=13
        name=parentname[:pre]+suf
        out.append((e,pre,ln,name,off,size,left,right))
        if right: walk(right,name)
        if not left: return
        parentname=name
walk(0,'')
for r in out[:N]: print('toc@%04x pre=%2d len=%2d %-40s off=%08x size=%8d left=%d right=%04x'%r)
print('entries',len(out),'files',sum(1 for r in out if r[5]))
mx=max(r[4]+r[5] for r in out); import os; print('max end',base+mx,'filesize',os.path.getsize(p))
