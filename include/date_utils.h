#ifndef HMS_DATE_UTILS_H
#define HMS_DATE_UTILS_H

#include <cstdint>

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