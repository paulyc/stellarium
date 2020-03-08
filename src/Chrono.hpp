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

struct jd_clock : public std::chrono::steady_clock
{
  typedef std::chrono::duration<long double, std::ratio<1,86400>> duration;
  typedef duration::rep                                 rep;
  typedef duration::period                              period;
  typedef std::chrono::time_point<jd_clock, duration> 	time_point;

    /*
     * delta-T =
          ET - UT   prior to 1984
          TDT - UT  1984 - 2000
          TT - UT   from 2001 and on

          delta-UT =  UT - UTC
          unixtime: no leap seconds, so UT
          */
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
  static constexpr bool is_steady = true;
  static constexpr long double SECONDS_PER_JDAY = 86400.0l;
  static constexpr long double JDAYS_PER_YEAR = 365.25l;
  static constexpr long double SECONDS_PER_JYEAR = SECONDS_PER_JDAY * JDAYS_PER_YEAR;
  static constexpr long double JD2000_EPOCH_JD = 2451545.0l;
  static constexpr long JD2000_EPOCH_JDS = 211813488000l;
  static constexpr long double JD2000_EPOCH_UNIXTIME = 946727935.816l;//946728000l;//
  static constexpr long double UNIX_EPOCH_JD = JD2000_EPOCH_JD - JD2000_EPOCH_UNIXTIME/SECONDS_PER_JDAY;
  static constexpr long double JD2000_DELTA_T = 63.83l;
  static constexpr long double DELTA_T_2018 = 68.97l;
  static constexpr long double leap_seconds_since_unix_epoch = 27.0l;

  static time_point now() noexcept {
      return from_unixtime(std::chrono::system_clock::now());
  }

  static time_point from_unixtime(const std::chrono::system_clock::time_point &t) {
      std::time_t unixtime = std::chrono::system_clock::to_time_t(t);
      return time_point(duration(UNIX_EPOCH_JD + unixtime/SECONDS_PER_JDAY));
  }

  struct YearDT {
      double year;
      double dt;
  };

  static const std::vector<YearDT> DELTA_T_YR;

  static double delta_t_lerp(double year) {
      size_t point = 1;
      while (point < jd_clock::DELTA_T_YR.size() - 1 && year > jd_clock::DELTA_T_YR[point].year) {
          ++point;
      }
      //std::cerr << "year " << year << std::endl;
      const double slope = (jd_clock::DELTA_T_YR[point].dt - jd_clock::DELTA_T_YR[point-1].dt) / (jd_clock::DELTA_T_YR[point].year - jd_clock::DELTA_T_YR[point-1].year);
      //std::cerr << "slope " << slope << std::endl;
      const double dy = jd_clock::DELTA_T_YR[point].year - year;
      //std::cerr << "dy " << dy << std::endl;
      const double dt = jd_clock::DELTA_T_YR[point].dt - dy * slope;
      //std::cerr << "dt " << dt << std::endl;
      return dt;
  }
  static void test_delta_t_lerp() {
      assert(delta_t_lerp(-700) > 17190);
      assert(delta_t_lerp(1250) > 200 && delta_t_lerp(1250) < 1570);
      assert(delta_t_lerp(2030) > 68.97);
  }
};

std::ostream& operator<<(std::ostream &os, const std::chrono::system_clock::time_point &rhs);
std::ostream& operator<<(std::ostream &os, const jd_clock::time_point &rhs);

#endif // CHRONO_HPP
