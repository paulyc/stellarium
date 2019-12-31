#ifndef CHRONO_HPP
#define CHRONO_HPP

#include <ctime>
#include <cmath>
#include <chrono>
#include <iostream>
#include <array>
#include <vector>
#include <tuple>
#include <cassert>
#include <iomanip>
#include <ratio>

#define DELTA_T_USE_MOON 0
#define DELTA_T_USE_STEL 0

#if DELTA_T_USE_STEL
#include "StelUtils.hpp"
#endif

class Chrono
{
public:
    Chrono();
};

typedef std::chrono::duration<long double, std::ratio<1,86400>> fracdays;
typedef std::chrono::duration<int64_t, std::ratio<86400>>  days;

struct julian_clock : public std::chrono::steady_clock
{
    // epoch = noon Universal Time (UT) Monday, 1 January 4713 BCE (-4712) Julian
    // J = 2000 + (Julian date − 2451545.0) ÷ 365.25
    // jd2000 epoch = 2451545.0 = TT 12:00:00.000 (UTC 11:58:55.816) 1 January 2000 CE Gregorian
    /*
     * The Gregorian date January 1, 2000, at 12:00 TT (Terrestrial Time).
The Julian date 2451545.0 TT (Terrestrial Time).[12]
January 1, 2000, 11:59:27.816 TAI (International Atomic Time).[13]
January 1, 2000, 11:58:55.816 UTC (Coordinated Universal Time).[14]
Smoothed historical measurements of ΔT using total solar eclipses are about
+17190 s in the year −500 (501 BC),
+10580 s in 0 (1 BC),
+5710 s in 500,
+1570 s in 1000, and
+200  s in 1500.
After the invention of the telescope, measurements were made by observing occultations of stars by the Moon,
which allowed the derivation of more closely spaced and more accurate values for ΔT.
ΔT continued to decrease until it reached a plateau of +11 ± 6 s between 1680 and 1866.
For about three decades immediately before 1902 it was negative, reaching -6.64 s. Then it increased to
+63.83 s in January 2000
+68.97 s in January 2018.
*/

    static constexpr int seconds_per_jday = 86400;
    static constexpr long double jdays_per_year = 365.25l;
    static constexpr long double seconds_per_jyear = seconds_per_jday * jdays_per_year;
    static constexpr long double JD2000_EPOCH_JD = 2451545.0l;
    static constexpr long JD2000_EPOCH_JDS = 211813488000l;
    static constexpr long JD2000_EPOCH_UNIXTIME = 946728000l;//946727935.816l;
    static constexpr long double UNIX_EPOCH_JD = JD2000_EPOCH_JD - JD2000_EPOCH_UNIXTIME/static_cast<long double>(seconds_per_jday);
    static constexpr long double JD2000_DELTA_T = 63.83l;
    static constexpr long double DELTA_T_2018 = 68.97l;
    static constexpr long double leap_seconds_since_unix_epoch = 27.0l;
    struct YearDT {
        double year;
        double dt;
    };

    static constexpr YearDT DELTA_T_YR[] = {
        {-500, 17190},
        {0, 10580},
        {500,5710},
        {1000,1570},
        {1500,200},
        {1900,-6.64},
        {2000,63.83},
        {2018,68.97}
    };
    static double delta_t_lerp(double year) {
        size_t point = 1;
        while (year > DELTA_T_YR[point].year && point < sizeof(DELTA_T_YR)) {
            ++point;
        }
        const double slope = (DELTA_T_YR[point].dt - DELTA_T_YR[point-1].dt) / (DELTA_T_YR[point].year - DELTA_T_YR[point-1].year);
        const double dy = DELTA_T_YR[point].year - year;
        const double dt = dy * slope + DELTA_T_YR[point].dt;
        return dt;
    }
    static void test_delta_t_lerp() {
        assert(delta_t_lerp(-700) > 17190);
        assert(delta_t_lerp(1250) > 200 && delta_t_lerp(1250) < 1570);
        assert(delta_t_lerp(2030) > 68.97);
    }
};

static inline std::ostream& operator<<(std::ostream &os, const std::chrono::system_clock::time_point &rhs)
{
    const std::time_t tt = std::chrono::system_clock::to_time_t(rhs);
    os << std::put_time(std::gmtime(&tt),"%FT%T%z (%Z)");
    return os;
}

struct jd_clock
{
  typedef std::chrono::duration<long double, std::ratio<1,86400>> 				duration;
  typedef duration::rep	  				rep;
  typedef duration::period	  				period;
  typedef std::chrono::time_point<jd_clock, duration> 	time_point;

  static constexpr bool is_steady = true;
  static constexpr int seconds_per_jday = 86400;
  static constexpr long double jdays_per_year = 365.25l;
  static constexpr long double seconds_per_jyear = seconds_per_jday * jdays_per_year;
  static constexpr long double JD2000_EPOCH_JD = 2451545.0l;
  static constexpr long JD2000_EPOCH_JDS = 211813488000l;
  static constexpr long JD2000_EPOCH_UNIXTIME = 946728000l;//946727935.816l;
  static constexpr long double UNIX_EPOCH_JD = JD2000_EPOCH_JD - JD2000_EPOCH_UNIXTIME/static_cast<long double>(seconds_per_jday);
  static constexpr long double JD2000_DELTA_T = 63.83l;
  static constexpr long double DELTA_T_2018 = 68.97l;
  static constexpr long double leap_seconds_since_unix_epoch = 27.0l;
  static time_point
  now() noexcept {
      return from_unixtime(std::chrono::system_clock::now());
  }
  static time_point from_unixtime(const std::chrono::system_clock::time_point &t) {
      std::time_t unixtime = std::chrono::system_clock::to_time_t(t);
      return time_point(duration((UNIX_EPOCH_JD + unixtime/86400.0)));
  }
};

static inline std::ostream& operator<<(std::ostream &os, const jd_clock::time_point &rhs)
{
    os << std::fixed << std::setw( 11 ) << std::setprecision( 6 )
       << std::setfill( '0' ) << rhs.time_since_epoch().count();
    return os;
}

#endif // CHRONO_HPP
