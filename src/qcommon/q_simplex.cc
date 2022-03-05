#include "q_simplex.hh"

#include <random>

#define PERM(v) perm[static_cast<uint8_t>(v)]

// disable for heuristic adjustment testing
static constexpr bool CLAMP_OUTPUTS = false;

// ================================================================================================================================
/*
███████╗██╗███╗   ███╗██████╗ ██╗     ███████╗██╗  ██╗    ██████╗ ██████╗ 
██╔════╝██║████╗ ████║██╔══██╗██║     ██╔════╝╚██╗██╔╝    ╚════██╗██╔══██╗
███████╗██║██╔████╔██║██████╔╝██║     █████╗   ╚███╔╝      █████╔╝██║  ██║
╚════██║██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝   ██╔██╗     ██╔═══╝ ██║  ██║
███████║██║██║ ╚═╝ ██║██║     ███████╗███████╗██╔╝ ██╗    ███████╗██████╔╝
╚══════╝╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝    ╚══════╝╚═════╝ 
*/
// ================================================================================================================================

static constexpr double HEURISTIC_2D = 45.23064;

static double const F2 = 0.5 * (sqrt(3.0) - 1.0);
static double const G2 = (3.0 - sqrt(3.0)) / 6.0;

static double grad2(uint8_t hash, double x, double y) {
    uint8_t h = hash & 0x3F;  // Convert low 3 bits of hash code
    const double u = h < 4 ? x : y;  // into 8 simple gradient directions,
    const double v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v); // and compute the dot product with (x,y).
}

double qm::simplex::generate(double x, double y) const {

	double n0 = 0.0, n1 = 0.0, n2 = 0.0;
	double s = (x + y) * F2;
	int_fast64_t i = std::floor(x + s);
	int_fast64_t j = std::floor(y + s);
	double t = (i + j) * G2;
	double X0 = i - t;
	double Y0 = j - t;
	double x0 = x - X0;
	double y0 = y - Y0;
	
	uint_fast8_t i1 = 0, j1 = 0;
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	}
	else {
		i1 = 0;
		j1 = 1;
	}
	
	double x1 = x0 - i1 + G2;
	double y1 = y0 - j1 + G2;
	double x2 = x0 - 1.0 + 2.0 * G2;
	double y2 = y0 - 1.0 + 2.0 * G2;
	
	int_fast16_t ii = static_cast<uint8_t>(i);
	int_fast16_t jj = static_cast<uint8_t>(j);
	uint8_t gi0 = PERM(ii      + PERM(jj     ));
	uint8_t gi1 = PERM(ii + i1 + PERM(jj + j1));
	uint8_t gi2 = PERM(ii + 1  + PERM(jj + 1 ));
	
	double t0 = 0.5 - x0 * x0 - y0 * y0;
	if (t0 < 0) n0 = 0.0;
	else {
		t0 *= t0;
		n0 = t0 * t0 * grad2(gi0, x0, y0);
	}
	double t1 = 0.5 - x1 * x1 - y1 * y1;
	if (t1 < 0) n1 = 0.0;
	else {
		t1 *= t1;
		n1 = t1 * t1 * grad2(gi1, x1, y1);
	}
	double t2 = 0.5 - x2 * x2 - y2 * y2;
	if (t2 < 0) n2 = 0.0;
	else {
		t2 *= t2;
		n2 = t2 * t2 * grad2(gi2, x2, y2);
	}
	
	double v = HEURISTIC_2D * (n0 + n1 + n2);
	if constexpr (CLAMP_OUTPUTS) qm::clamp(v, -1.0, 1.0);
	return v;
}

// ================================================================================================================================
/*
███████╗██╗███╗   ███╗██████╗ ██╗     ███████╗██╗  ██╗    ██████╗ ██████╗ 
██╔════╝██║████╗ ████║██╔══██╗██║     ██╔════╝╚██╗██╔╝    ╚════██╗██╔══██╗
███████╗██║██╔████╔██║██████╔╝██║     █████╗   ╚███╔╝      █████╔╝██║  ██║
╚════██║██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝   ██╔██╗      ╚═══██╗██║  ██║
███████║██║██║ ╚═╝ ██║██║     ███████╗███████╗██╔╝ ██╗    ██████╔╝██████╔╝
╚══════╝╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝    ╚═════╝ ╚═════╝ 
*/
// ================================================================================================================================

static constexpr double HEURISTIC_3D = 32.696;

static constexpr double F3 = 1.0 / 3.0;
static constexpr double G3 = 1.0 / 6.0;

static double grad3(int32_t hash, double x, double y, double z) {
    int h = hash & 15;     // Convert low 4 bits of hash code into 12 simple
    double u = h < 8 ? x : y; // gradient directions, and compute dot product.
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z; // Fix repeats at h = 12 to 15
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

double qm::simplex::generate(double x, double y, double z) const {
	
	double s = (x + y + z) * F3;
	int_fast64_t i = std::floor(x + s);
	int_fast64_t j = std::floor(y + s);
	int_fast64_t k = std::floor(z + s);
	double t = (i + j + k) * G3;
	
	qm::vec3d_t c0 {
		x - (i - t),
		y - (j - t),
		z - (k - t)
	};
	
	int_fast64_t i1, j1, k1;
	int_fast64_t i2, j2, k2;
	if (c0[0] >= c0[1]) {
		if (c0[1] >= c0[2]) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} else if (c0[0] >= c0[2]) {
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} else {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		}
	} else {	
		if (c0[1] < c0[2]) {
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} else if (c0[0] < c0[2]) {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} else {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		}
	}
	
	qm::vec3d_t
		c1 {
			c0[0] - i1 + G3,
			c0[1] - j1 + G3,
			c0[2] - k1 + G3,
		},
		c2 {
			c0[0] - i2 + G3 * 2,
			c0[1] - j2 + G3 * 2,
			c0[2] - k2 + G3 * 2,
		},
		c3 {
			c0[0] - 1  + G3 * 3,
			c0[1] - 1  + G3 * 3,
			c0[2] - 1  + G3 * 3,
		};
	
	int_fast64_t ii = static_cast<uint8_t>(i);
	int_fast64_t jj = static_cast<uint8_t>(j);
	int_fast64_t kk = static_cast<uint8_t>(k);
	int_fast64_t gi0 = PERM(ii +      PERM(jj +      PERM(kk     )));
	int_fast64_t gi1 = PERM(ii + i1 + PERM(jj + j1 + PERM(kk + k1)));
	int_fast64_t gi2 = PERM(ii + i2 + PERM(jj + j2 + PERM(kk + k2)));
	int_fast64_t gi3 = PERM(ii + 1  + PERM(jj + 1  + PERM(kk + 1 )));
	
	double n0, n1, n2, n3;
	
	t = 0.6 - c0.dot(c0);
	if (t < 0) n0 = 0.0;
	else {
		n0 = std::pow(t, 4) * grad3(gi0, c0[0], c0[1], c0[2]);
	}
	t = 0.6 - c1.dot(c1);
	if (t < 0) n1 = 0.0;
	else {
		n1 = std::pow(t, 4) * grad3(gi1, c1[0], c1[1], c1[2]);
	}
	t = 0.6 - c2.dot(c2);
	if (t < 0) n2 = 0.0;
	else {
		n2 = std::pow(t, 4) * grad3(gi2, c2[0], c2[1], c2[2]);
	}
	t = 0.6 - c3.dot(c3);
	if (t < 0) n3 = 0.0;
	else {
		n3 = std::pow(t, 4) * grad3(gi3, c3[0], c3[1], c3[2]); 
	}
	
	double v = HEURISTIC_3D * (n0 + n1 + n2 + n3);
	if constexpr (CLAMP_OUTPUTS) qm::clamp(v, -1.0, 1.0);
	return v;
}

// ================================================================================================================================
/*
███████╗██╗███╗   ███╗██████╗ ██╗     ███████╗██╗  ██╗    ██╗  ██╗██████╗ 
██╔════╝██║████╗ ████║██╔══██╗██║     ██╔════╝╚██╗██╔╝    ██║  ██║██╔══██╗
███████╗██║██╔████╔██║██████╔╝██║     █████╗   ╚███╔╝     ███████║██║  ██║
╚════██║██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝   ██╔██╗     ╚════██║██║  ██║
███████║██║██║ ╚═╝ ██║██║     ███████╗███████╗██╔╝ ██╗         ██║██████╔╝
╚══════╝╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝         ╚═╝╚═════╝ 
*/
// ================================================================================================================================

static constexpr double HEURISTIC_4D = 27.228;

static constexpr qm::vec4d_t GRAD4[] = {
	{0,1,1,1},  {0,1,1,-1},  {0,1,-1,1},  {0,1,-1,-1},
	{0,-1,1,1}, {0,-1,1,-1}, {0,-1,-1,1}, {0,-1,-1,-1},
	{1,0,1,1},  {1,0,1,-1},  {1,0,-1,1},  {1,0,-1,-1},
	{-1,0,1,1}, {-1,0,1,-1}, {-1,0,-1,1}, {-1,0,-1,-1},
	{1,1,0,1},  {1,1,0,-1},  {1,-1,0,1},  {1,-1,0,-1},
	{-1,1,0,1}, {-1,1,0,-1}, {-1,-1,0,1}, {-1,-1,0,-1},
	{1,1,1,0},  {1,1,-1,0},  {1,-1,1,0},  {1,-1,-1,0},
	{-1,1,1,0}, {-1,1,-1,0}, {-1,-1,1,0}, {-1,-1,-1,0}
};

static double const F4 = (sqrt(5.0) - 1.0) / 4.0;
static double const G4 = (5.0 - sqrt(5.0)) / 20.0;

double qm::simplex::generate(double x, double y, double z, double w) const {
	
	double s = (x + y + z + w) * F4;
	
	std::array<int64_t, 4> skew {
		static_cast<int64_t>(std::floor(x + s)),
		static_cast<int64_t>(std::floor(y + s)),
		static_cast<int64_t>(std::floor(z + s)),
		static_cast<int64_t>(std::floor(w + s))
	};
		
	double t = (skew[0] + skew[1] + skew[2] + skew[3]) * G4;
	qm::vec4d_t c0 {
		x - (skew[0] - t),
		y - (skew[1] - t),
		z - (skew[2] - t),
		w - (skew[3] - t)
	};
		
	std::array<uint8_t, 4> ranks {0, 0, 0, 0};
	ranks[c0[0] > c0[1] ? 0 : 1]++;
	ranks[c0[0] > c0[2] ? 0 : 2]++;
	ranks[c0[0] > c0[3] ? 0 : 3]++;
	ranks[c0[1] > c0[2] ? 1 : 2]++;
	ranks[c0[1] > c0[3] ? 1 : 3]++;
	ranks[c0[2] > c0[3] ? 2 : 3]++;
	
	qm::vec4d_t 
		c1 {
			c0[0] - (ranks[0] >= 3 ? 1 : 0) + G4,
			c0[1] - (ranks[1] >= 3 ? 1 : 0) + G4,
			c0[2] - (ranks[2] >= 3 ? 1 : 0) + G4,
			c0[3] - (ranks[3] >= 3 ? 1 : 0) + G4
		},
		c2 {
			c0[0] - (ranks[0] >= 2 ? 1 : 0) + G4 * 2,
			c0[1] - (ranks[1] >= 2 ? 1 : 0) + G4 * 2,
			c0[2] - (ranks[2] >= 2 ? 1 : 0) + G4 * 2,
			c0[3] - (ranks[3] >= 2 ? 1 : 0) + G4 * 2
		},
		c3 {
			c0[0] - (ranks[0] >= 1 ? 1 : 0) + G4 * 3,
			c0[1] - (ranks[1] >= 1 ? 1 : 0) + G4 * 3,
			c0[2] - (ranks[2] >= 1 ? 1 : 0) + G4 * 3,
			c0[3] - (ranks[3] >= 1 ? 1 : 0) + G4 * 3
		},
		c4 {
			c0[0] - 1 /*      \  /       */ + G4 * 4,
			c0[1] - 1 /*    O      O     */ + G4 * 4,
			c0[2] - 1 /*  \__________/   */ + G4 * 4,
			c0[3] - 1 /*     \_/         */ + G4 * 4
		};
		
	std::array<int_fast16_t, 4> si {
		static_cast<int_fast16_t>(static_cast<uint8_t>(skew[0])),
		static_cast<int_fast16_t>(static_cast<uint8_t>(skew[1])),
		static_cast<int_fast16_t>(static_cast<uint8_t>(skew[2])),
		static_cast<int_fast16_t>(static_cast<uint8_t>(skew[3]))
	};
	
	std::array<int_fast64_t, 5> gi {
		static_cast<int_fast64_t>(PERM(si[0]                           + PERM(si[1]                           + PERM(si[2]                           + PERM(si[3]                          )))) % 32),
		static_cast<int_fast64_t>(PERM(si[0] + (ranks[0] >= 3 ? 1 : 0) + PERM(si[1] + (ranks[1] >= 3 ? 1 : 0) + PERM(si[2] + (ranks[2] >= 3 ? 1 : 0) + PERM(si[3] + (ranks[3] >= 3 ? 1 : 0))))) % 32),
		static_cast<int_fast64_t>(PERM(si[0] + (ranks[0] >= 2 ? 1 : 0) + PERM(si[1] + (ranks[1] >= 2 ? 1 : 0) + PERM(si[2] + (ranks[2] >= 2 ? 1 : 0) + PERM(si[3] + (ranks[3] >= 2 ? 1 : 0))))) % 32),
		static_cast<int_fast64_t>(PERM(si[0] + (ranks[0] >= 1 ? 1 : 0) + PERM(si[1] + (ranks[1] >= 1 ? 1 : 0) + PERM(si[2] + (ranks[2] >= 1 ? 1 : 0) + PERM(si[3] + (ranks[3] >= 1 ? 1 : 0))))) % 32),
		static_cast<int_fast64_t>(PERM(si[0] + 1                       + PERM(si[1] + 1                       + PERM(si[2] + 1                       + PERM(si[3] + 1                      )))) % 32)
	};
	
	std::array<double, 5> nc;
	
	t = 0.6 - qm::vec4d_t::dot(c0, c0);
	nc[0] = t < 0 ? 0 : std::pow(t, 4) * qm::vec4d_t::dot(GRAD4[gi[0]], c0);
	t = 0.6 - qm::vec4d_t::dot(c1, c1);
	nc[1] = t < 0 ? 0 : std::pow(t, 4) * qm::vec4d_t::dot(GRAD4[gi[1]], c1);
	t = 0.6 - qm::vec4d_t::dot(c2, c2);
	nc[2] = t < 0 ? 0 : std::pow(t, 4) * qm::vec4d_t::dot(GRAD4[gi[2]], c2);
	t = 0.6 - qm::vec4d_t::dot(c3, c3);
	nc[3] = t < 0 ? 0 : std::pow(t, 4) * qm::vec4d_t::dot(GRAD4[gi[3]], c3);
	t = 0.6 - qm::vec4d_t::dot(c4, c4);
	nc[4] = t < 0 ? 0 : std::pow(t, 4) * qm::vec4d_t::dot(GRAD4[gi[4]], c4);
	
	double v = HEURISTIC_4D * (nc[0] + nc[1] + nc[2] + nc[3] + nc[4]);
	if constexpr (CLAMP_OUTPUTS) qm::clamp(v, -1.0, 1.0);
	return v;
}
