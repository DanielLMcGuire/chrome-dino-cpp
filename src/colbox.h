#pragma once

struct CollisionBox { int x, y, w, h; };

inline CollisionBox adjustedBox(const CollisionBox& box, const CollisionBox& origin) {
    return {box.x + origin.x, box.y + origin.y, box.w, box.h};
}

inline bool boxesOverlap(const CollisionBox& a, const CollisionBox& b) {
    return a.x < b.x + b.w  &&
           a.x + a.w > b.x  &&
           a.y < b.y + b.h  &&
           a.y + a.h > b.y;
}