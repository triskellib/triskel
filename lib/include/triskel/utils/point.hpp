#pragma once

namespace triskel {

struct Point {
    float x;
    float y;
};

inline auto operator+(const Point& l, const Point& r) -> Point {
    return {.x = l.x + r.x, .y = l.y + r.y};
}

inline auto operator+=(Point& l, const Point& r) -> Point& {
    l = l + r;
    return l;
}

}  // namespace triskel