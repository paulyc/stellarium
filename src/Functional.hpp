#ifndef FUNCTIONAL_HPP
#define FUNCTIONAL_HPP

#include <functional>
#include <optional>

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

#endif // FUNCTIONAL_HPP
