#include <iostream>
#include <iomanip>
#include <memory>
#include <exception>
#include <optional>
#include <chrono>
#include <ctime>
#include <array>
#include <variant>
#include <functional>
#include <cstdarg>
#include <cstring>
#include <string>

#include "jpleph.h"
#include "de430.hpp"
#include "de431.hpp"
#include "StelUtils.hpp"

struct jpl_eph_data;

#if 0
class JPLEphError : public std::exception
{
public:
    JPLEphError() : _errcode(jpl_init_error_code()), _errmsg(jpl_init_error_message()) {}
private:
    int _errcode;
    const char *_errmsg;
};
#endif

template<typename Left_T, typename Right_T>
class Either
{
private:
    enum Which {
        Left,
        Right,
    } _which;
    union {
        Left_T left;
        Right_T right;
    } _value;
public:
    Either() = delete;
    explicit Either(Left_T left) : _which(Left), _value(left) {}
    explicit Either(Right_T right) : _which(Right), _value(right) {}

    bool has_left() const { return _which == Left; }
    bool has_right() const { return _which == Right; }
    std::optional<const Left_T&> try_left() const { return has_left() ? std::make_optional(&_value.left) : std::nullopt; }
    std::optional<const Right_T&> try_right() const { return has_right() ? std::make_optional(&_value.right) : std::nullopt; }

    static constexpr std::function<Left_T(Left_T)> LeftIdentityFun = [](Left_T left) {return left;};
    static constexpr std::function<Right_T(Right_T)> RightIdentityFun = [](Right_T right) {return right;};

    template <typename Left_Map_T, typename Right_Map_T>
    Either<Left_Map_T, Right_Map_T> map(const std::function<Left_Map_T(Left_T)> &lfun, const std::function<Right_Map_T(Right_T)> &rfun)
    {
        if (has_left()) {
            return Either<Left_Map_T, Right_Map_T>(lfun(_value.left));
        } else {
            return Either<Left_Map_T, Right_Map_T>(rfun(_value.right));
        }
    }

    template <typename Map_T>
    Either<Map_T, Right_T> map_left(const std::function<Map_T(Left_T)> &fun)
    {
        return this->map(fun, RightIdentityFun);
    }
    template <typename Map_T>
    Either<Left_T, Map_T> map_right(const std::function<Map_T(Right_T)> &fun)
    {
        return this->map(LeftIdentityFun, fun);
    }
private:

};

std::string string_format(const char *format, ...)
{
    va_list args;
    va_start (args, format);
    int len = std::vsnprintf(nullptr, 0, format, args);
    if (len < 0) {
        throw std::runtime_error(strerror(errno));
    }
    va_end (args);
    const size_t slen = static_cast<size_t>(len) + 1;
    std::string formatted(slen, '\0');
    va_start (args, format);
    std::vsnprintf(formatted.data(), slen, format, args);
    va_end (args);
    return formatted;
}

typedef double vec3d_t[3];
typedef vec3d_t pv_t[2];
typedef std::array<long double, 2> vec2q_t;

long double normalize(vec3d_t &v)
{
    const long double x = static_cast<long double>(v[0]);
    const long double y = static_cast<long double>(v[1]);
    const long double z = static_cast<long double>(v[2]);
    const long double mag = sqrtl(x*x+y*y+z*z);
    v[0] = static_cast<double>(x/mag);
    v[1] = static_cast<double>(y/mag);
    v[2] = static_cast<double>(z/mag);
    return mag;
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
        long double get_ra() const {
            vec3d_t normal = {position[0], position[1], position[2]};
            normalize(normal);
            return atanl(static_cast<long double>(normal[1]) / static_cast<long double>(normal[0]));
        }
        vec2q_t magphase(const State &other) {
            vec2q_t magphase;
            vec3d_t this_normal = {position[0], position[1], position[2]};
            normalize(this_normal);
            vec3d_t other_normal = {other.position[0], other.position[1], other.position[2]};
            normalize(other_normal);
            vec3d_t sum = {this_normal[0] + other_normal[0], this_normal[1] + other_normal[1], this_normal[2] + other_normal[2]};
            magphase[0] = sqrtl(static_cast<long double>(sum[0])*static_cast<long double>(sum[0])+static_cast<long double>(sum[1])*static_cast<long double>(sum[1]));
            magphase[1] = atanl(static_cast<long double>(sum[1]) / static_cast<long double>(sum[0]));
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

#define DELTA_T_USE_MOON 0
struct julian_clock : public std::chrono::steady_clock
{
    // epoch = noon Universal Time (UT) Monday, 1 January 4713 BCE Julian
    // jd2000 epoch = 2451545.0 = TT 12:00:00.000 (UTC 11:58:55.816) 1 January 2000 CE Gregorian
    static constexpr double JD2000_EPOCH = 2451545.0;
    //static constexpr bool DELTA_T_USE_MOON = true;
    // Espenak & Meeus (2006) algorithm for DeltaT
    static constexpr double deltaTnDot = -25.858; // n.dot = -25.858 "/cy/cy
    //static constexpr const std::function<double(double)> deltaTfunc = StelUtils::getDeltaTByEspenakMeeus;
    static constexpr int deltaTstart	= -1999;
    static constexpr int deltaTfinish	= 3000;
    typedef std::chrono::seconds duration;
    typedef std::chrono::time_point<julian_clock, duration> 	time_point;

    static long double point_to_jde(const time_point &tp) {
        return tp.time_since_epoch().count() / 86400.0l;
    }

    static time_point now() noexcept {
        const double jd = StelUtils::getJDFromSystem();
        const double deltat_now = StelUtils::getDeltaTByEspenakMeeus(jd);

#if DELTA_T_USE_MOON
        if (DELTA_T_USE_MOON) {
            deltat += StelUtils::getMoonSecularAcceleration(jd, deltaTnDot, ((de430Active&&EphemWrapper::jd_fits_de430(JD)) || (de431Active&&EphemWrapper::jd_fits_de431(JD))));
        }
#endif

        const std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
        const std::time_t unixtime = std::chrono::system_clock::to_time_t(t); // seconds since 1970-01-01T00:00:00.000Z
        const long double unixjdays = unixtime / (86400.0l * 365.25l);
        const long double jdunixepoch = static_cast<long double>(jd) - unixjdays;
        const long double deltat_unixepoch = static_cast<long double>(StelUtils::getDeltaTByEspenakMeeus(static_cast<double>(jdunixepoch)));
        const long double deltat_unixtime = static_cast<long double>(deltat_now) - deltat_unixepoch;
        const long double jde = static_cast<long double>(jd) + deltat_unixtime/86400.0l;
        return time_point(std::chrono::seconds(static_cast<int64_t>(floorl(jde * 86400.0l))));
    }
};

std::ostream& operator<<(std::ostream &os, const std::chrono::system_clock::time_point &rhs)
{
    const std::time_t tt = std::chrono::system_clock::to_time_t(rhs);
    os << std::put_time(std::gmtime(&tt),"%FT%T%z (%Z)");
    return os;
}

int main()
{
    JPLEphems _de430;
    JPLEphems _de430t;
    JPLEphems _de431;

   //std::chrono::steady_clock::time_point
    std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
    std::cout << "hello newmoon " << t << std::endl;

    try {
        _de430.init("/home/paulyc/Development/stellarium/ephem/linux_p1550p2650.430");
        _de430t.init("/home/paulyc/Development/stellarium/ephem/linux_p1550p2650.430t");
        _de431.init("/home/paulyc/Development/stellarium/ephem/lnxm13000p17000.431");
    } catch (std::exception &ex) {
        std::cerr << "EXCEPTION: " <<  ex.what() << std::endl;
        return 1;
    }

    julian_clock::time_point now = julian_clock::now();

    // just find the time when its the minimum
    long double minmag = 2.0l;
    std::chrono::system_clock::time_point mintp = std::chrono::system_clock::now();
    for (int i = 0; i < 29*24*60; ++i) {
        now += std::chrono::seconds(60);
        t += std::chrono::seconds(60);
        const double jde_now = static_cast<double>(julian_clock::point_to_jde(now));
        JPLEphems::State sm = _de431.get_state(jde_now, JPLEphems::Earth, JPLEphems::Moon);
        JPLEphems::State ss = _de431.get_state(jde_now, JPLEphems::Sun, JPLEphems::EarthMoonBarycenter);
        vec2q_t magphase = sm.magphase(ss);
        std::cout << "Magnitude " << magphase[0] << " Angle " << magphase[1] << " at date " << t << std::endl;
        if (magphase[0] < minmag) {
            minmag = magphase[0];
            mintp = t;
        }
    }
    std::cout << "minmag " << minmag << " at time " << mintp << std::endl;

    std::cout << "goodbye newmoon" << std::endl;
    return 0;
}
