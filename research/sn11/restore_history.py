"""Restore pre-2011 traversal semantics without changing model policies."""
from pathlib import Path
p=Path("snowpack/Stability.cc")
s=p.read_text()
start=s.index("void Stability::findWeakLayer(")
a,b=s[:start],s[start:]
for threshold in ("Stability::minimum_slab","Pk"):
    old="while ((e-- > Xdata.SoilNode) && (((Xdata.cH - (NDS[e+1].z + NDS[e+1].u))/cos_sl) < "+threshold+")) {};"
    new="while ((e > Xdata.SoilNode+1) && (((Xdata.cH - (NDS[e].z + NDS[e].u))/cos_sl) < "+threshold+")) { --e; }"
    assert b.count(old)==1,old
    b=b.replace(old,new)
old="if (e==static_cast<size_t>(-1)) e=0; //HACK: this is ugly: e got corrupted if SoilNode==0"
assert b.count(old)==2
b=b.replace("\t"+old+"\n","").replace("\t"+old+"\n","")
# The cursor is now one above the current element. Preserve old e > SoilNode fallback.
old="if ((e > Xdata.SoilNode) && (e != IOUtils::unodata)) {"
assert b.count(old)==2
b=b.replace(old,"if (e > Xdata.SoilNode+1) {")
# No unsigned wraparound; consume the selected candidate exactly once.
for condition in (
    "((NDS[e+1].z + NDS[e+1].u)/cos_sl > Stability::ground_rough)",
    "(((Xdata.cH - (NDS[e+1].z + NDS[e+1].u))/cos_sl) < (Pk + Stability::skier_depth)) && ((NDS[e+1].z + NDS[e+1].u)/cos_sl > Stability::ground_rough)"
):
    old="while ((e-- > Xdata.SoilNode) && "+condition+") {"
    new="while (e > Xdata.SoilNode) {\n"+("\t\t\t" if condition.startswith("((NDS") else "\t\t\t\t")+"--e;\n"+("\t\t\t" if condition.startswith("((NDS") else "\t\t\t\t")+"if (!("+condition+")) break;"
    assert b.count(old)==1,old
    b=b.replace(old,new)
# Remove any whitespace-only line left by deletion.
b=b.replace("\n\t\n","\n\n")
p.write_text(a+b)
