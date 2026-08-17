#ifndef HMS_DATE_UTILS_H
#define HMS_DATE_UTILS_H

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdio>
#include <chrono>
#include <ctime>

namespace hms::dateutils
{

    inline int64_t daysFromCivil(int y, unsigned m, unsigned d)
    {
        y -= (m <= 2) ? 1 : 0;

        const int64_t era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe =
            static_cast<unsigned>(y - era * 400);

        const unsigned doy =
            (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;

        const unsigned doe =
            yoe * 365 + yoe / 4 - yoe / 100 + doy;

        return era * 146097 +
               static_cast<int64_t>(doe) - 719468;
    }
    inline void civilFromDays(int64_t z, int &y, unsigned &m, unsigned &d)
    {
        z += 719468;

        const int64_t era =
            (z >= 0 ? z : z - 146096) / 146097;

        const unsigned doe =
            static_cast<unsigned>(z - era * 146097);

        const unsigned yoe =
            (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;

        const int y2 =
            static_cast<int>(yoe) +
            static_cast<int>(era * 400);

        const unsigned doy =
            doe - (365 * yoe + yoe / 4 - yoe / 100);

        const unsigned mp =
            (5 * doy + 2) / 153;

        d = doy - (153 * mp + 2) / 5 + 1;
        m = mp + (mp < 10 ? 3 : -9);
        y = y2 + ((m <= 2) ? 1 : 0);
    }

    inline std::string formatDate(int y, unsigned m, unsigned d)
    {
        std::ostringstream oss;

        oss << std::setfill('0') << std::setw(4) << y << "-"
            << std::setfill('0') << std::setw(2) << m << "-"
            << std::setfill('0') << std::setw(2) << d;

        return oss.str();
    }
    inline std::string today()
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTimeT =
            std::chrono::system_clock::to_time_t(now);

        std::tm utcTm{};

#if defined(_WIN32)
        gmtime_s(&utcTm, &nowTimeT);
#else
        gmtime_r(&nowTimeT, &utcTm);
#endif

        return formatDate(
            utcTm.tm_year + 1900,
            static_cast<unsigned>(utcTm.tm_mon + 1),
            static_cast<unsigned>(utcTm.tm_mday));
    }
    inline std::string today()
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTimeT =
            std::chrono::system_clock::to_time_t(now);

        std::tm utcTm{};

#if defined(_WIN32)
        gmtime_s(&utcTm, &nowTimeT);
#else
        gmtime_r(&nowTimeT, &utcTm);
#endif

        return formatDate(
            utcTm.tm_year + 1900,
            static_cast<unsigned>(utcTm.tm_mon + 1),
            static_cast<unsigned>(utcTm.tm_mday));
    }
    inline std::string addDays(const std::string &dateStr, int days)
    {
        int y;
        unsigned m, d;

        parseDate(dateStr, y, m, d);

        const int64_t z =
            daysFromCivil(y, m, d) + days;

        civilFromDays(z, y, m, d);

        return formatDate(y, m, d);
    }
    inline bool isPast(const std::string &dateStr)
    {
        return dateStr < today();
    }

} // namespace hms::dateutils

#endif // HMS_DATE_UTILS_H