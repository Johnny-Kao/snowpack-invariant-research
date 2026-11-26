// Research-only native probe. The CI build exposes the private selector
// through a header access-specifier change; no selection logic is modified.
#include <snowpack/DataClasses.h>
#include <snowpack/Stability.h>
#include <cmath>
#include <iostream>
#include <vector>
struct Case {const char* name; size_t soil; size_t weak; double penetration;};
int main(){
  const Case cases[]={{"first eligible",0,7,.1},{"deeper control",0,6,.1},
                      {"near-ground control",0,1,.15},{"same snow over soil",1,1,.15}};
  int failures=0;
  for(const auto& c:cases){
    SnowStation x(false,c.soil!=0);
    x.resize(8+c.soil);
    x.SoilNode=c.soil; x.Ground=c.soil*.5; x.cH=x.Ground+1.; x.cos_sl=1.;
    for(size_t j=0;j<x.Ndata.size();++j){
      auto& n=x.Ndata[j];
      n.z=j<c.soil?0.:x.Ground+(j-c.soil)*.125;
      n.u=0.; n.S_n=n.S_s=n.ssi=3.;
    }
    for(size_t j=0;j<x.Edata.size();++j){
      x.Edata[j].L=j<c.soil?.5:.125; x.Edata[j].S_dr=3.;
    }
    const size_t w=c.soil+c.weak;
    x.Ndata[w].S_n=x.Ndata[w].S_s=x.Ndata[w].ssi=.5; x.Edata[w-1].S_dr=.5;
    std::vector<unsigned short> lemons(x.Ndata.size(),0);
    double ssi,sk; size_t lemon;
    Stability::findWeakLayer(c.penetration,lemons,x,ssi,sk,lemon);
    double natural=6., skier=6.;
    for(size_t j=c.soil+1;j<x.Ndata.size();++j){
      const double h=x.Ndata[j].z-x.Ground, depth=x.cH-x.Ndata[j].z;
      if(h>Stability::ground_rough && depth>=Stability::minimum_slab)
        natural=std::min(natural,x.Ndata[j].S_n);
      if(h>Stability::ground_rough && depth>=c.penetration && depth<c.penetration+Stability::skier_depth)
        skier=std::min(skier,x.Ndata[j].ssi);
    }
    bool ok=std::abs(x.S_n-natural)<1e-12 && std::abs(x.S_d-natural)<1e-12
      && std::abs(x.S_s-skier)<1e-12 && std::abs(x.S_4-skier)<1e-12;
    failures+=!ok;
    std::cout<<c.name<<": natural="<<x.S_n<<" skier="<<x.S_s<<" structural="<<x.S_4
      <<" expected="<<natural<<"/"<<skier<<" "<<(ok?"PASS":"FAIL")<<"\n";
  }
  return failures;
}
