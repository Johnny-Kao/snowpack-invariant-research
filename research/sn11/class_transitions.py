import csv,sys,collections
def load(p):
    with open(p) as f:
        return {tuple(r[1:3]):int(float(r[-1])) for r in csv.reader(f) if r and r[0]=='SN11'}
total=collections.Counter()
for a,b in zip(sys.argv[1::2],sys.argv[2::2]):
    x,y=load(a),load(b)
    assert x.keys()==y.keys()
    changes=collections.Counter((x[k],y[k]) for k in x if x[k]!=y[k])
    print(a,len(x),dict(sorted(changes.items())))
    total.update(changes)
print("combined",dict(sorted(total.items())))
print("valid grade changes",sum(v for (a,b),v in total.items() if a in (1,3,5) and b in (1,3,5)))
print("undefined transitions",sum(v for (a,b),v in total.items() if -1 in (a,b)))
