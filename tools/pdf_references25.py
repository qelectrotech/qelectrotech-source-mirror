# SPDX-License-Identifier: GPL-2.0-or-later
"""Extract PDF navigation and page artwork. Never execute PDF actions."""
import sys,json,re,math
from pathlib import Path
import pymupdf as fitz

def extract(source,destination):
    out=Path(destination);out.mkdir(parents=True,exist_ok=True)
    document=fitz.open(source)
    if document.needs_pass:raise ValueError("Password-protected PDFs must be unlocked before import.")
    pages=[];links=[];printed=[];issues=[]
    for i,page in enumerate(document):
        page.get_pixmap(matrix=fitz.Matrix(2,2),alpha=False).save(out/f"page-{i}.png")
        pages.append({"index":i,"label":page.get_label(),"width":page.rect.width,"height":page.rect.height,"image":f"page-{i}.png"})
        resolved={}
        entry=page.first_link
        while entry:
            if entry.uri and entry.uri.startswith('#') and not entry.is_external:
                try:
                    target,x,y=document.resolve_link(entry.uri)
                    if 0<=target<len(document):
                        target_rect=document[target].rect;centre=(target_rect.tl+target_rect.br)/2
                        resolved[entry.xref]=(target,fitz.Point(x if math.isfinite(x) else centre.x,y if math.isfinite(y) else centre.y))
                except (ValueError,RuntimeError):pass
            entry=entry.next
        for link in page.get_links():
            if link['kind']==fitz.LINK_NAMED and link.get('xref') in resolved:
                target,point=resolved[link['xref']]
                links.append({'sourcePage':i,'sourceRect':list(link['from']),'targetPage':target,'targetPoint':list(point),'kind':'pdf'})
                continue
            if link["kind"]==fitz.LINK_GOTO and 0<=link.get("page",-1)<len(document):
                point=link.get("to",fitz.Point(0,0))
                # get_links() already returns rotated page coordinates; text
                # extraction below does not. Do not rotate links a second time.
                rect=link["from"]
                target=point
                links.append({"sourcePage":i,"sourceRect":list(rect),"targetPage":link["page"],"targetPoint":list(target),"kind":"pdf"})
            elif link["kind"] not in (fitz.LINK_NONE,):
                issues.append({"page":i,"reason":"External, named or unsupported destination retained for review; no external action imported."})
        for word in page.get_text("words"):
            match=re.fullmatch(r"/([A-Za-z0-9_-]+)\.([0-9]+)",word[4])
            if match:
                rect=fitz.Rect(word[:4])*page.rotation_matrix
                printed.append({"sourcePage":i,"sourceRect":list(rect),"sheet":match[1],"column":int(match[2]),"text":word[4]})
    manifest={"version":1,"pages":pages,"links":links,"printed":printed,"issues":issues}
    (out/"manifest.json").write_text(json.dumps(manifest,ensure_ascii=False),encoding="utf-8")
    return manifest

if __name__=="__main__":
    extract(sys.argv[1],sys.argv[2])

