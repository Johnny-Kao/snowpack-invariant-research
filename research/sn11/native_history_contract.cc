// Native comparison against the pre-2011 signed-index traversal, with
// contemporary scoring, thresholds, fallback and nodata policies preserved.
#include <snowpack/DataClasses.h>
#include <snowpack/Stability.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>
using namespace std;
struct Result {double d,n,s,ssi,zd,zn,zs,zssi,swssi,swsk;size_t lemon;};
static Result historical(const SnowStation& x,double pk,const vector<unsigned short>& lemons) {
  const auto& nodes=x.Ndata;const auto& elements=x.Edata;
  const int soil=static_cast<int>(x.SoilNode), ne=static_cast<int>(elements.size());
  const double missing=mio::IOUtils::nodata;
  Result r{missing,missing,missing,missing,x.cH,x.cH,x.cH,x.cH,missing,missing,0};
  auto depth=[&](int e){return (x.cH-nodes[e+1].z-nodes[e+1].u)/x.cos_sl;};
  auto above=[&](int e){return (nodes[e+1].z+nodes[e+1].u)/x.cos_sl>Stability::ground_rough;};
  int e=ne-1;
  while(e>soil && depth(e)<Stability::minimum_slab) --e;
  if(e>soil){
    for(;e>=soil && above(e);--e){
      if(r.d==missing || r.d>elements[e].S_dr){
        r.d=elements[e].S_dr;r.zd=(nodes[e].z+nodes[e+1].z+nodes[e].u+nodes[e+1].u)/2.;
      }
      if(r.n==missing || r.n>nodes[e+1].S_n){r.n=nodes[e+1].S_n;r.zn=nodes[e+1].z+nodes[e+1].u;}
    }
    r.zd-=x.Ground;r.zn-=x.Ground;
  }else{r.d=elements[soil].S_dr;r.n=nodes[soil+1].S_n;r.zd=r.zn=elements[soil].L;}
  if(x.cH/x.cos_sl>Stability::ground_rough && x.cH/x.cos_sl-pk>Stability::min_depth_ssi){
    e=ne-1;
    while(e>soil && depth(e)<pk)--e;
    if(e>soil){
      for(;e>=soil && depth(e)<pk+Stability::skier_depth && above(e);--e){
        const auto& n=nodes[e+1];
        if(r.swssi==missing||r.swssi>n.ssi||(fabs(r.swssi-n.ssi)<.09&&lemons[e+1]>r.lemon)){
          r.swssi=n.ssi;r.swsk=n.S_s;r.lemon=lemons[e+1];r.zs=r.zssi=n.z+n.u;
        }
      }
      r.s=r.swsk;r.ssi=r.swssi;r.zs-=x.Ground;r.zssi-=x.Ground;
    }else{r.s=nodes[soil+1].S_s;r.ssi=nodes[soil+1].ssi;r.zs=r.zssi=elements[soil].L;}
  }else{r.s=Stability::max_stability;r.ssi=nodes[ne].ssi;r.zs=r.zssi=x.cH;}
  return r;
}
int main(int argc,char**){
  mt19937 rng(61203);size_t count=0,fail=0;
  for(size_t soil:{0u,1u,3u})for(size_t n:{1u,2u,3u,4u,8u,32u})for(double cos_sl:{1.,.8})for(double pk:{0.,.0625,.125,.25,.5,1.,3.})for(int rep=0;rep<20;++rep){
    SnowStation x(false,soil>0);x.resize(soil+n);x.SoilNode=soil;x.Ground=soil*.125;x.cos_sl=cos_sl;
    double z=0.;x.Ndata[0].z=0.;x.Ndata[0].u=0.;
    vector<unsigned short> lemons(soil+n+1,0);
    for(size_t j=0;j<soil+n;++j){
      auto& e=x.Edata[j];auto& node=x.Ndata[j+1];
      e.L=j<soil?.125:((rep%2==0)?.125:(rng()%8+1)*.015625);z+=e.L;node.z=z;node.u=0.;
      e.S_dr=(rng()%100+1)*.05;node.S_n=(rng()%100+1)*.05;node.S_s=(rng()%100+1)*.05;node.ssi=node.S_s+(rng()%3);lemons[j+1]=rng()%3;
    }
    x.cH=z;
    x.Ndata.back().ssi=Stability::max_stability;
    const Result ref=historical(x,pk,lemons);
    double ssi,sk;size_t lemon;
    Stability::findWeakLayer(pk,lemons,x,ssi,sk,lemon);
    const double got[]={x.S_d,x.S_n,x.S_s,x.S_4,x.z_S_d,x.z_S_n,x.z_S_s,x.z_S_4,ssi,sk};
    const double exp[]={ref.d,ref.n,ref.s,ref.ssi,ref.zd,ref.zn,ref.zs,ref.zssi,ref.swssi,ref.swsk};
    bool pass=lemon==ref.lemon;
    for(int i=0;i<10;++i)pass=pass&&fabs(got[i]-exp[i])<1e-10;
    ++count;fail+=!pass;
    if(!pass && fail<=5)cout<<"mismatch soil="<<soil<<" n="<<n<<" pk="<<pk<<" rep="<<rep<<"\n";
  }
  cout<<"historical traversal contract: "<<count<<" cases, "<<fail<<" mismatches\n";
  return argc>1?(fail==0?0:1):(fail>0?0:1);
}
