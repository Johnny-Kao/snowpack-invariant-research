"""Compare identical season time/slope keys and quantify actual output changes."""
import csv,sys,json,math,statistics
from pathlib import Path
def load(path):
    out={}
    for r in csv.reader(Path(path).open()):
        if not r or r[0]!="SN11": continue
        assert len(r)==15,r
        key=tuple(r[1:3]); assert key not in out,key
        out[key]=[float(v) for v in r[3:]]
    assert out,path
    return out
def compare(a,b):
    x,y=load(a),load(b)
    assert x.keys()==y.keys(),(len(x),len(y))
    assert all(x[k][:3]==y[k][:3] for k in x),"physical trace differs"
    report={"snow_covered_steps":len(x)}
    labels=["natural","deformation","skier","structural","natural_height","deformation_height","skier_height","structural_height","class2"]
    for i,label in enumerate(labels,3):
        paired=[(x[k][i],y[k][i]) for k in x]
        valid=[v-u for u,v in paired if u!=-999 and v!=-999 and math.isfinite(u) and math.isfinite(v)]
        delta=[d for d in valid if abs(d)>1e-6]
        report[label]={"changed":sum(abs(u-v)>1e-6 for u,v in paired),"valid_pairs":len(valid),
                       "lower":sum(d<0 for d in delta),"higher":sum(d>0 for d in delta),
                       "max_abs_delta":max(map(abs,delta),default=0),"median_abs_changed":statistics.median(map(abs,delta)) if delta else 0,
                       "missing_status_changes":sum((u==-999)!=(v==-999) for u,v in paired)}
    return report
for base,variant in zip(sys.argv[1::2],sys.argv[2::2]):
    print(json.dumps({"baseline":base,"candidate":variant,**compare(base,variant)},indent=2))
