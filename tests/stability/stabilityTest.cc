#include <snowpack/Stability.h>
#include <snowpack/Constants.h>
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

namespace {
	const double tol = 1.e-10;

	bool equal(const double a, const double b)
	{
		return std::isfinite(a) && std::isfinite(b) && fabs(a-b) < tol;
	}

	// Use the indices calculated by checkStability, independently selecting
	// from all eligible interfaces rather than repeating its reverse cursor.
	bool checkProfile(Stability& stability, const CurrentMeteo& meteo,
	                  const size_t n, const double dz, const double cos_sl,
	                  const size_t weak, size_t& first_selected)
	{
		SnowStation profile(false, false);
		profile.resize(n);
		profile.SoilNode = 0;
		profile.Ground = 0.;
		profile.cH = n*dz;
		profile.cos_sl = cos_sl;
		for (size_t i=0; i<=n; ++i) {
			profile.Ndata[i].z = i*dz;
			profile.Ndata[i].u = 0.;
			profile.Ndata[i].T = 263.15;
		}
		double mass = 0.;
		for (size_t i=n; i>0; ) {
			--i;
			ElementData& el = profile.Edata[i];
			el.L = el.L0 = dz;
			el.Rho = (i==weak)? 120. : 400.;
			el.M = el.Rho*dz;
			el.C = -Constants::g*cos_sl*(mass+0.5*el.M);
			mass += el.M;
			el.Te = 263.15;
			el.theta[ICE] = el.Rho/Constants::density_ice;
			el.theta[AIR] = 1.-el.theta[ICE];
			el.rg = (i==weak)? 0.8 : 0.3;
			el.rb = el.rg/3.;
			el.N3 = 3.;
			el.type = (i==weak)? 440 : 330;
			el.res_wat_cont = 0.03;
		}
		const double pk = StabilityAlgorithms::compPenetrationDepth(profile);
		stability.checkStability(meteo, profile);

		// Test only profiles with a nonempty search range above the bottom
		// element. The existing shallow-profile fallback is a separate rule.
		vector<size_t> natural, skier;
		for (size_t i=1; i<=n; ++i) {
			const double height = profile.Ndata[i].z/cos_sl;
			const double depth = (profile.cH-profile.Ndata[i].z)/cos_sl;
			if (height > Stability::ground_rough) {
				if (depth >= Stability::minimum_slab) natural.push_back(i);
				if (depth >= pk && depth < pk+Stability::skier_depth) skier.push_back(i);
			}
		}
		if (natural.empty() || natural.back()==1 || skier.empty() || skier.back()==1 ||
		    profile.cH/cos_sl-pk <= Stability::min_depth_ssi) {
			cerr << "Invalid weak-layer test profile: thickness=" << dz
			     << ", cos(slope)=" << cos_sl << ", penetration=" << pk << endl;
			return false;
		}

		// Keep the upper interface in a tie, as in the documented search.
		size_t ni = natural.back(), di = ni;
		for (auto it=natural.rbegin(); it!=natural.rend(); ++it) {
			if (profile.Ndata[*it].S_n < profile.Ndata[ni].S_n) ni = *it;
			if (profile.Edata[*it-1].S_dr < profile.Edata[di-1].S_dr) di = *it;
		}
		size_t si = skier.back();
		auto lemons = [&](const size_t i) {
			if (i==n) return 0;
			const ElementData& a = profile.Edata[i-1];
			const ElementData& b = profile.Edata[i];
			return static_cast<int>(fabs(a.hard-b.hard)>1.5)
			     + static_cast<int>(2.*fabs(a.rg-b.rg)>0.5);
		};
		for (auto it=skier.rbegin(); it!=skier.rend(); ++it) {
			const double candidate = profile.Ndata[*it].ssi;
			const double selected = profile.Ndata[si].ssi;
			if (candidate < selected ||
			    (fabs(candidate-selected)<0.09 && lemons(*it)>lemons(si))) si = *it;
		}
		if (si==skier.back()) ++first_selected;
		const bool ok = equal(profile.S_n, profile.Ndata[ni].S_n)
		             && equal(profile.z_S_n, profile.Ndata[ni].z)
		             && equal(profile.S_d, profile.Edata[di-1].S_dr)
		             && equal(profile.z_S_d, 0.5*(profile.Ndata[di-1].z+profile.Ndata[di].z))
		             && equal(profile.S_s, profile.Ndata[si].S_s)
		             && equal(profile.S_4, profile.Ndata[si].ssi)
		             && equal(profile.z_S_s, profile.Ndata[si].z)
		             && equal(profile.z_S_4, profile.Ndata[si].z);
		if (!ok) cerr << "Wrong weak layer: layers=" << n << ", thickness=" << dz
		              << ", cos(slope)=" << cos_sl << ", weak element=" << weak
		              << ", expected SSI height=" << profile.Ndata[si].z
		              << ", selected=" << profile.z_S_4 << endl;
		return ok;
	}
}

int main(int argc, char *argv[])
{
	if (argc!=2) return 1;
	const SnowpackConfig cfg(argv[1]);
	Stability stability(cfg);
	CurrentMeteo meteo(cfg);
	size_t failures = 0, first_selected = 0, count = 0;
	for (const double cos_sl : {1., 0.8}) {
		// Keep the upper element within the 0.3 m penetration-density window.
		for (const double dz : {0.0625, 0.125, 0.1875}) {
			const size_t n = 16;
			for (size_t weak=0; weak<n; ++weak) {
				++count;
				if (!checkProfile(stability, meteo, n, dz, cos_sl, weak, first_selected)) ++failures;
			}
		}
	}
	cout << count << " stability profiles, " << first_selected
	     << " with the first eligible skier interface selected, " << failures << " failures" << endl;
	return (failures==0 && first_selected>0)? 0 : 1;
}
