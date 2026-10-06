#pragma once

struct CollisionBox { int x, y, w, h; };

struct BoxSpan {
    const CollisionBox* data  = nullptr;
    int                 count = 0;

    [[nodiscard]] const CollisionBox* begin() const { return data; }
    [[nodiscard]] const CollisionBox* end()   const { return data + count; }
    [[nodiscard]] int  size()  const { return count; }
    [[nodiscard]] bool empty() const { return count == 0; }
    const CollisionBox& operator[](int i) const { return data[i]; }
};

inline CollisionBox adjustedBox(const CollisionBox& box, const CollisionBox& origin) {
    return {box.x + origin.x, box.y + origin.y, box.w, box.h};
}

inline bool boxesOverlap(const CollisionBox& a, const CollisionBox& b) {
    return a.x < b.x + b.w  &&
           a.x + a.w > b.x  &&
           a.y < b.y + b.h  &&
           a.y + a.h > b.y;
}