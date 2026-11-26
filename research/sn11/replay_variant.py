"""Research-only replay instrumentation and narrow selector variants."""
import sys
from pathlib import Path
p=Path("snowpack/Stability.cc")
s=p.read_text()
mode=sys.argv[1]
if mode=="trace":
    anchor="\tif (classify_profile) {"
    assert s.count(anchor)==1
    trace=r'''    // Research trace, no mutation of physical or classification state.
    std::cout << "SN11," << Mdata.date.toString(mio::Date::ISO) << "," << Xdata.sector
              << "," << nE-Xdata.SoilNode << "," << Xdata.cH-Xdata.Ground << "," << Pk
              << "," << Xdata.S_n << "," << Xdata.S_d << "," << Xdata.S_s << "," << Xdata.S_4
              << "," << Xdata.z_S_n << "," << Xdata.z_S_d << "," << Xdata.z_S_s << "," << Xdata.z_S_4
              << "," << Xdata.S_class2 << "\n";
'''
    # class fields may be signed char; output numerically.
    trace=trace.replace("<< Xdata.S_class2", "<< static_cast<int>(Xdata.S_class2)")
    s=s.replace(anchor,trace+anchor)
elif mode in ("first","both"):
    start=s.index("void Stability::findWeakLayer(")
    a,b=s[:start],s[start:]
    # Preserve next candidate for the consuming loop; don't change fallback policy.
    for threshold in ("Stability::minimum_slab","Pk"):
        old="while ((e-- > Xdata.SoilNode) && (((Xdata.cH - (NDS[e+1].z + NDS[e+1].u))/cos_sl) < "+threshold+")) {};"
        new="while ((e > Xdata.SoilNode) && (((Xdata.cH - (NDS[e].z + NDS[e].u))/cos_sl) < "+threshold+")) { --e; }"
        assert b.count(old)==1,old
        b=b.replace(old,new)
    if mode=="both":
        old="((NDS[e+1].z + NDS[e+1].u)/cos_sl > Stability::ground_rough)"
        assert b.count(old)==2
        b=b.replace(old,"((NDS[e+1].z + NDS[e+1].u - Xdata.Ground)/cos_sl > Stability::ground_rough)")
        assert b.count("Xdata.cH/cos_sl")==2
        b=b.replace("Xdata.cH/cos_sl","(Xdata.cH-Xdata.Ground)/cos_sl")
    s=a+b
else:
    raise ValueError(mode)
p.write_text(s)
