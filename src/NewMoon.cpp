/*
 * Stellarium
 * Copyright (C) 2019 Paul Ciarlo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <iostream>
#include <iomanip>
#include <memory>
#include <exception>
#include <chrono>
#include <ctime>
#include <array>
#include <variant>

#include "jpleph.h"
#include "de430.hpp"
#include "de431.hpp"
#include "StelUtils.hpp"

#include "NewMoon.hpp"
#include "Chrono.hpp"

/*
 * delta-T =
      ET - UT   prior to 1984
      TDT - UT  1984 - 2000
      TT - UT   from 2001 and on

      delta-UT =  UT - UTC
      unixtime: no leap seconds, so UT
      */
struct jpl_eph_data;

typedef double vec3d_t[3];
typedef vec3d_t pv_t[2];
typedef std::array<long double, 2> vec2q_t;
typedef std::array<long double, 3> vec3q_t;

vec3q_t normalize(const vec3d_t &v)
{
    const long double x = static_cast<long double>(v[0]);
    const long double y = static_cast<long double>(v[1]);
    const long double z = static_cast<long double>(v[2]);
    const long double mag = sqrtl(x*x+y*y+z*z);
   // v[0] = static_cast<double>(x/mag);
   // v[1] = static_cast<double>(y/mag);
   // v[2] = static_cast<double>(z/mag);
    return {x/mag,y/mag,z/mag};
}

class JPLEphems
{
    static constexpr size_t MAX_CONSTANTS = 1024;

public:
    enum Point {
        Mercury =  1,
        Venus   =  2,
        Earth   =  3,
        Mars    =  4,
        Jupiter =  5,
        Saturn  =  6,
        Uranus  =  7,
        Neptune =  8,
        Pluto   =  9,
        Moon    = 10,
        Sun     = 11,
        SolarSystemBarycenter = 12,
        EarthMoonBarycenter   = 13,
        Nutations             = 14,
        Librations            = 15,
        LunarMantleOmega      = 16,
        TT_TDB                = 17,
    };

    struct State
    {
        union {
            double pv[6];
            vec3d_t position;
            vec3d_t velocity;
        };
        vec2q_t ra_magphase(const State &other) {
            vec2q_t magphase;
            vec3d_t this_ = {position[0], position[1], position[2]};
            vec3q_t this_normal = normalize(this_);
            vec3d_t that = {other.position[0], other.position[1], other.position[2]};
            vec3q_t that_normal = normalize(that);
            vec3q_t sum = {this_normal[0] + that_normal[0], this_normal[1] + that_normal[1], this_normal[2] + that_normal[2]};
            magphase[0] = sqrtl(sum[0]*sum[0]+sum[1]*sum[1]);
            magphase[1] = atanl(sum[1] / sum[0]);
            return magphase;
        }
    };
    JPLEphems() = default;
    ~JPLEphems()
    {
        if (_ephdata != nullptr) {
            jpl_close_ephemeris(_ephdata);
            _ephdata = nullptr;
        }
    }

    void init(const std::string &filename)
    {
        _ephdata = static_cast<jpl_eph_data*>(jpl_init_ephemeris(filename.c_str(), _names, _values));
        if (_ephdata == nullptr) {
            throw std::runtime_error(jpl_init_error_message());
        }
    }
    State get_state(double jdt, Point center, Point ref)
    {
        State result;
        int res = jpl_pleph(_ephdata, jdt, ref, center, result.pv, 0);
        if (res != 0) {
            throw std::runtime_error(string_format("jpl_pleph returned code %d", res));
        }
        return result;
    }
private:
    char _names[MAX_CONSTANTS][6];
    double _values[MAX_CONSTANTS];
    jpl_eph_data *_ephdata;
};

int run()
{
    JPLEphems _de430;
    JPLEphems _de430t;
    JPLEphems _de431;

    char tzbuf[] = "TZ=UTC";
    putenv(tzbuf);
  //  julian_clock::test_delta_t_lerp();

    std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
    jd_clock::time_point jd = jd_clock::now();

    std::cout << "hello newmoon " << t << std::endl;
    timespec ts = {5,0}; //5.000000000s
    try {
        _de430.init("/home/paulyc/Development/stellarium/ephem/linux_p1550p2650.430");
        _de430t.init("/home/paulyc/Development/stellarium/ephem/linux_p1550p2650.430t");
        _de431.init("/home/paulyc/Development/stellarium/ephem/lnxm13000p17000.431");
    } catch (std::exception &ex) {
        std::cerr << "EXCEPTION: " <<  ex.what() << std::endl;
        return 1;
    }

    // generate newmoons every ts seconds forever
    for (;;) {
        // just find the time when its the minimum
        long double lastmags[2] = {-3.0l};
        size_t lastmag_indx = 0;
        long double minmag = 3.0l;
        std::chrono::system_clock::time_point mintp = std::chrono::system_clock::now();
        for (int i = 0; i < 29*24*60; ++i) {
            jd += fracdays(60.0/86400.0);
            t += std::chrono::seconds(60);
            //const double jd_now = static_cast<double>(julian_clock::point_to_jde(jd));
            const double jd_now = static_cast<double>(fracdays(jd.time_since_epoch()).count());
            JPLEphems::State sm = _de431.get_state(jd_now, JPLEphems::Earth, JPLEphems::Moon);
            JPLEphems::State ss = _de431.get_state(jd_now, JPLEphems::Sun, JPLEphems::EarthMoonBarycenter);
            vec2q_t magphase = sm.ra_magphase(ss);
            const long double mag = magphase[0];
            const long double lastmag = lastmags[lastmag_indx % 2];
            ++lastmag_indx;
            lastmags[lastmag_indx % 2] = mag;
            const long double dmag = mag - lastmag;
            //std::cout << "Magnitude " << magphase[0] << " Angle " << magphase[1] << " at date " << t << std::endl;
            if (mag < minmag && dmag < 0.0l) {
                minmag = mag;
                mintp = t;
            } else if (mag > 1.0l && minmag < 0.01l && dmag > 0.0l) {
                break;
            }
        }
        std::cout << "minmag (newmoon) " << minmag << " at mintp " << mintp << std::endl;
        nanosleep(&ts, nullptr);
    }

    std::cout << "goodbye newmoon" << std::endl;
    return 0;
}

int test() {
    //julian_clock::time_point tp = julian_clock::now();
    auto jd = jd_clock::now();
    std::cout << jd_clock::now() << std::endl;
    std::cout << (double)jd.time_since_epoch().count() << std::endl;
    const double jd_now = static_cast<double>(fracdays(jd.time_since_epoch()).count());
    std::cout << jd_now << std::endl;
    return 0;
}

int main() {
#if 1
    return run();
#else
    return test();
#endif
}
