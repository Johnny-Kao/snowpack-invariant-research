// Focused regression probe for SnowStation::averageFromTop.
#include <snowpack/DataClasses.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
struct Layer { double thickness, water; };
struct Case { const char* name; std::vector<Layer> layers; size_t soil; double depth; };
static double reference(const Case& x) {
    double H=0., z=0., integral=0., covered=0.;
    for (size_t i=x.soil; i<x.layers.size(); ++i) H+=x.layers[i].thickness;
    for (size_t i=x.soil; i<x.layers.size(); ++i) {
        const auto& l=x.layers[i];
        const double w=std::max(0., std::min(H,z+l.thickness)-std::max(0.,std::max(H-x.depth,z)));
        integral+=w*l.water; covered+=w; z+=l.thickness;
    }
    return covered>0. ? integral/covered : 0.;
}
int main() {
    const std::vector<Case> cases={
        {"single wet",{{.2,.025}},0,.3},
        {"shallow two-layer wet",{{.1,.025},{.1,.025}},0,.3},
        {"soil plus single wet",{{.5,.25},{.2,.025}},1,.3},
        {"deep uniform",{{.2,.025},{.2,.025},{.2,.025},{.2,.025},{.2,.025}},0,.3},
        {"deep varied",{{.2,.01},{.2,.02},{.2,.03},{.2,.04}},0,.3},
        {"exact-depth control",{{.2,.01},{.3,.025}},0,.3},
        {"dry control",{{.1,0.},{.1,0.}},0,.3}
    };
    int failures=0;
    for (const auto& x:cases) {
        SnowStation station(false,x.soil>0);
        station.resize(x.layers.size());
        station.SoilNode=x.soil;
        for(size_t i=0;i<x.layers.size();++i) {
            station.Edata[i].L=x.layers[i].thickness;
            station.Edata[i].theta[WATER]=x.layers[i].water;
            station.Edata[i].theta[WATER_PREF]=0.;
            station.Edata[i].theta[ICE]=i<x.soil?0.:.25;
            station.Edata[i].theta[SOIL]=i<x.soil?.5:0.;
            station.Edata[i].theta[AIR]=1.-station.Edata[i].theta[ICE]-station.Edata[i].theta[SOIL]-station.Edata[i].theta[WATER];
        }
        const double got=station.averageFromTop(x.depth,[](const ElementData& e){return e.theta[WATER]+e.theta[WATER_PREF];});
        const double expected=reference(x);
        const bool pass=std::abs(got-expected)<1.e-12;
        failures+=!pass;
        std::cout<<std::setprecision(12)<<x.name<<": got="<<got<<" reference="<<expected<<" "<<(pass?"PASS":"FAIL")<<"\n";
    }
    return failures;
}
