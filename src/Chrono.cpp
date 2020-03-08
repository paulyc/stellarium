#include "Chrono.hpp"

const std::vector<jd_clock::YearDT> jd_clock::DELTA_T_YR = {
    {-500, 17190},
    {0, 10580},
    {500,5710},
    {1000,1570},
    {1500,200},
    {1900,-6.64},
    {2000,63.83},
    {2018,68.97}
};

std::ostream& operator<<(std::ostream &os, const std::chrono::system_clock::time_point &rhs)
{
    const std::time_t tt = std::chrono::system_clock::to_time_t(rhs);
    os << std::put_time(std::gmtime(&tt),"%FT%T%z (%Z)");
    return os;
}

std::ostream& operator<<(std::ostream &os, const jd_clock::time_point &rhs)
{
    os << std::fixed << std::setw( 11 ) << std::setprecision( 6 )
       << std::setfill( '0' ) << rhs.time_since_epoch().count();
    return os;
}
