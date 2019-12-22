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

#include <functional>
#include <optional>
#include <string>
#include <cstring>
#include <cstdarg>

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
