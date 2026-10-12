"""Mirror of Grok Drop / GMD-API import parsing: handlePlistDataForParsing + DS_Dictionary root dict + k4 level string."""
import sys, re, base64, gzip, xml.etree.ElementTree as ET
def handle(value):
    value=value.replace("\0"," ")
    if '<plist version="1.0">' in value[:100]: value=value.replace('<plist version="1.0">','<plist version="1.0" gjver="2.0">',1)
    old=False
    if not value.startswith("<?xml version"):
        if "<plist version" not in value[:100]:
            old=True; value='<?xml version="1.0"?><plist version="1.0" gjver="2.0"><dict><k>root</k>'+value+"</dict></plist>"
        else: value='<?xml version="1.0"?>'+value
    return value,old
def plist_dict(el):
    out={}; it=list(el)
    for k,v in zip(it[::2],it[1::2]):
        t=v.text or ""
        out[k.text]=(plist_dict(v) if v.tag in ("d","dict") else t if v.tag=="s" else int(t or 0) if v.tag=="i"
                     else float(t or 0) if v.tag=="r" else v.tag=="t")
    return out
def from_txt(path):
    s=open(path).read().strip()
    if not s.startswith("H4sI"):
        assert ";" in s, "not a level string"
        s=base64.urlsafe_b64encode(gzip.compress(s.encode())).decode()
    name=path.rsplit("/",1)[-1].rsplit(".",1)[0]
    return f'<?xml version="1.0"?><plist version="1.0" gjver="2.0"><dict><k>kCEK</k><i>4</i><k>k2</k><s>{name}</s><k>k4</k><s>{s}</s></dict></plist>'
def parse(path):
    raw=from_txt(path) if path.lower().endswith(".txt") else open(path,encoding="utf-8",errors="replace").read()
    value,old=handle(raw)
    root=ET.fromstring(value.encode())
    d=plist_dict(root.find("dict"))
    if "root" in d and isinstance(d["root"],dict): d=d["root"]   # DS_Dictionary stepIntoSubDictWithKey("root") (old files)
    k4=d.get("k4","")
    ls=gzip.decompress(base64.urlsafe_b64decode(k4+"="*(-len(k4)%4))).decode()
    objs=[o for o in ls.split(";")[1:] if o]
    return d,ls,objs,old
if __name__=="__main__":
    for p in sys.argv[1:]:
        d,ls,objs,old=parse(p)
        print(f"{p}: name={d.get('k2')!r} song={d.get('k8')} kCEK={d.get('kCEK')} oldfile={old} objects={len(objs)} header={ls.split(';')[0][:40]}... k72={'k72' in d}")
