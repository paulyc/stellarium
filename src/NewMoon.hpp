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

#ifndef NEWMOON_HPP
#define NEWMOON_HPP

#include <functional>
#include <optional>
#include <string>
#include <cstring>
#include <cstdarg>

inline static std::string string_format(const char *format, ...)
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

#endif // NEWMOON_HPP
