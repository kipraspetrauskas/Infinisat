"""Download CC0 assets from Poly Haven (glTF models, texture maps) for Infinisat.
Usage: python -I polyhaven_dl.py <out_dir> models:<id,id,..> textures:<id,id,..>
"""
import json, os, sys, urllib.request
UA = {"User-Agent": "Mozilla/5.0 (hackathon asset fetcher)"}
def get(url):
    return urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=60).read()
def save(url, path):
    if os.path.exists(path) and os.path.getsize(path) > 0: return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "wb").write(get(url))
out = sys.argv[1]
for arg in sys.argv[2:]:
    kind, ids = arg.split(":", 1)
    for aid in [x for x in ids.split(",") if x]:
        try:
            files = json.loads(get(f"https://api.polyhaven.com/files/{aid}"))
            if kind == "models":
                g = files["gltf"]
                res = "1k" if "1k" in g else sorted(g)[0]
                e = g[res]["gltf"]
                base = os.path.join(out, "models", aid)
                save(e["url"], os.path.join(base, os.path.basename(e["url"])))
                for rel, inc in e.get("include", {}).items():
                    save(inc["url"], os.path.join(base, rel))
                print("ok model", aid, res)
            else:
                base = os.path.join(out, "textures", aid)
                for m, key in (("Diffuse", "diff"), ("nor_gl", "nor"), ("Rough", "rough"), ("AO", "ao")):
                    if m in files and "1k" in files[m]:
                        ent = files[m]["1k"].get("jpg") or files[m]["1k"].get("png")
                        if ent: save(ent["url"], os.path.join(base, f"{aid}_{key}.{ent['url'].rsplit('.',1)[1]}"))
                print("ok tex", aid)
        except Exception as ex:
            print("FAIL", kind, aid, ex)
