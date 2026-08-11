#pragma once
// mini_phys2d.hpp
#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <set>
#include <optional>
#include <sstream>

namespace mini2d {

    // ---------------------------------------------------------------- Vec2 -----
    struct Vec2 {
        float x = 0.f, y = 0.f;
        Vec2() = default;
        Vec2(float x_, float y_) : x(x_), y(y_) {}
        Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
        Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
        Vec2 operator-() const { return { -x, -y }; }
        Vec2 operator*(float s) const { return { x * s, y * s }; }
        Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
        Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
        float length() const { return std::sqrt(x * x + y * y); }
        Vec2 normalized() const { float l = length(); return l > 1e-8f ? Vec2{ x / l, y / l } : Vec2{ 0,0 }; }
        std::string str() const { std::ostringstream ss; ss << "(" << x << ", " << y << ")"; return ss.str(); }
    };
    inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
    inline float cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }   // r x v -> scalar
    inline Vec2  cross(float w, const Vec2& r) { return { -w * r.y, w * r.x }; }          // w x r -> vec
    inline Vec2  rotate(const Vec2& v, float ang) {
        float c = std::cos(ang), s = std::sin(ang);
        return { v.x * c - v.y * s, v.x * s + v.y * c };
    }
    constexpr float PI = 3.14159265358979323846f;

    // -------------------------------------------------------------- Shapes -----
    enum class ShapeType { Circle, Box, Edge };
    struct Shape2D {
        ShapeType type;
        float radius = 0.f;         // circle
        float hw = 0.f, hh = 0.f;   // box half-extents
        Vec2  normal{ 0,1 };        // edge: outward normal (points into free space)
        float offset = 0.f;         // edge: plane is { p : dot(n,p) == offset }
    };
    inline Shape2D shape2DCircle(float r) { Shape2D s; s.type = ShapeType::Circle; s.radius = r; return s; }
    inline Shape2D shape2DBox(float hw, float hh) { Shape2D s; s.type = ShapeType::Box; s.hw = hw; s.hh = hh; return s; }
    inline Shape2D shape2DEdge(Vec2 n, float offset) { Shape2D s; s.type = ShapeType::Edge; s.normal = n.normalized(); s.offset = offset; return s; }

    // --------------------------------------------------------------- Body ------
    struct Body2D { int id = -1; bool valid() const { return id >= 0; } };

    struct BodyData {
        Shape2D shape;
        Vec2  pos{ 0,0 };
        float angle = 0.f;
        Vec2  vel{ 0,0 };
        float angVel = 0.f;
        float mass = 1.f, invMass = 0.f;
        float inertia = 1.f, invInertia = 0.f;
        float friction = 0.3f, restitution = 0.3f;
        bool  isStatic = true;
    };

    struct Spring {
        int a, b;
        Vec2 la, lb;   // local anchor offsets (in body frame, from centre)
        float rest, k, d;
    };

    struct WorldConfig {
        Vec2 gravity{ 0.f, -9.81f };
        bool enableSleeping = true;
    };
    struct RawWorld {
        WorldConfig cfg;
        WorldConfig& config() { return cfg; }
    };

    struct Contact {
        int a, b;
        Vec2 normal;   // points from a -> b
        Vec2 point;
        float depth;
    };

    class World2D {
    public:
        World2D() = default;

        Body2D addStatic(Shape2D shape, Vec2 pos, float angle = 0.f) {
            BodyData bd; bd.shape = shape; bd.pos = pos; bd.angle = angle;
            bd.isStatic = true; bd.invMass = 0.f; bd.invInertia = 0.f;
            bodies_.push_back(bd);
            return { (int)bodies_.size() - 1 };
        }

        Body2D addDynamic(Shape2D shape, Vec2 pos, float angle, float mass, float friction, float restitution) {
            BodyData bd; bd.shape = shape; bd.pos = pos; bd.angle = angle;
            bd.isStatic = false; bd.mass = mass; bd.invMass = mass > 0.f ? 1.f / mass : 0.f;
            bd.friction = friction; bd.restitution = restitution;
            if (shape.type == ShapeType::Circle)
                bd.inertia = 0.5f * mass * shape.radius * shape.radius;
            else if (shape.type == ShapeType::Box)
                bd.inertia = mass * (4.f * shape.hw * shape.hw + 4.f * shape.hh * shape.hh) / 12.f;
            else
                bd.inertia = mass;
            bd.invInertia = bd.inertia > 0.f ? 1.f / bd.inertia : 0.f;
            bodies_.push_back(bd);
            return { (int)bodies_.size() - 1 };
        }

        void addSpring(Body2D a, Body2D b, Vec2 la, Vec2 lb, float rest, float k, float d) {
            springs_.push_back({ a.id, b.id, la, lb, rest, k, d });
            ignoreCollision(a, b); // adjacent spring-linked bodies shouldn't fight the spring via contacts
        }

        void setLinearVelocity(Body2D b, Vec2 v) { bodies_[b.id].vel = v; }
        void setAngularVelocity(Body2D b, float w) { bodies_[b.id].angVel = w; }
        Vec2 getPosition(Body2D b) const { return bodies_[b.id].pos; }
        float getAngle(Body2D b) const { return bodies_[b.id].angle; }
        const BodyData& data(Body2D b) const { return bodies_[b.id]; }

        void ignoreCollision(Body2D a, Body2D b) {
            int i = std::min(a.id, b.id), j = std::max(a.id, b.id);
            ignored_.insert({ i, j });
        }

        RawWorld& raw() { return raw_; }

        int activeBodies() const {
            int n = 0; for (auto& b : bodies_) if (!b.isStatic) ++n; return n;
        }

        const std::vector<BodyData>& bodies() const { return bodies_; }

        void step(float dt) {
            // 1) integrate forces (gravity) for dynamic bodies
            for (auto& b : bodies_) {
                if (b.isStatic) continue;
                b.vel += raw_.cfg.gravity * dt;
            }
            // 2) springs
            applySprings(dt);
            // 3) integrate velocities -> positions
            for (auto& b : bodies_) {
                if (b.isStatic) continue;
                b.pos += b.vel * dt;
                b.angle += b.angVel * dt;
                // light damping keeps things from jittering forever
                b.vel = b.vel * 0.9995f;
                b.angVel *= 0.998f;
            }
            // 4) collision detection + sequential impulse solver
            auto contacts = findContacts();
            for (int iter = 0; iter < 8; ++iter)
                for (auto& c : contacts) resolveVelocity(c);
            for (auto& c : contacts) positionalCorrection(c);
        }

    private:
        std::vector<BodyData> bodies_;
        std::vector<Spring> springs_;
        std::set<std::pair<int, int>> ignored_;
        RawWorld raw_;

        void applySprings(float dt) {
            for (auto& s : springs_) {
                auto& A = bodies_[s.a]; auto& B = bodies_[s.b];
                Vec2 wa = A.pos + rotate(s.la, A.angle);
                Vec2 wb = B.pos + rotate(s.lb, B.angle);
                Vec2 delta = wb - wa;
                float len = delta.length();
                if (len < 1e-6f) continue;
                Vec2 dir = delta * (1.f / len);
                float stretch = len - s.rest;

                Vec2 rvA = A.vel + cross(A.angVel, wa - A.pos);
                Vec2 rvB = B.vel + cross(B.angVel, wb - B.pos);
                float relVel = dot(rvB - rvA, dir);

                float forceMag = s.k * stretch + s.d * relVel;
                Vec2 force = dir * forceMag;

                if (!A.isStatic) {
                    A.vel += force * (A.invMass * dt);
                    A.angVel += A.invInertia * cross(wa - A.pos, force) * dt;
                }
                if (!B.isStatic) {
                    B.vel -= force * (B.invMass * dt);
                    B.angVel -= B.invInertia * cross(wb - B.pos, force) * dt;
                }
            }
        }

        static std::vector<Vec2> boxCorners(const BodyData& b) {
            std::vector<Vec2> c(4);
            Vec2 local[4] = { {b.shape.hw,b.shape.hh},{-b.shape.hw,b.shape.hh},{-b.shape.hw,-b.shape.hh},{b.shape.hw,-b.shape.hh} };
            for (int i = 0; i < 4; ++i) c[i] = b.pos + rotate(local[i], b.angle);
            return c;
        }

        bool isIgnored(int i, int j) const {
            return ignored_.count({ std::min(i, j), std::max(i, j) }) > 0;
        }

        std::vector<Contact> findContacts() {
            std::vector<Contact> out;
            int n = (int)bodies_.size();
            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    auto& A = bodies_[i]; auto& B = bodies_[j];
                    if (A.isStatic && B.isStatic) continue;
                    if (isIgnored(i, j)) continue;
                    collidePair(i, A, j, B, out);
                }
            }
            return out;
        }

        void collidePair(int i, const BodyData& A, int j, const BodyData& B, std::vector<Contact>& out) {
            using T = ShapeType;
            if (A.shape.type == T::Circle && B.shape.type == T::Circle) circleCircle(i, A, j, B, out);
            else if (A.shape.type == T::Circle && B.shape.type == T::Box) circleBox(i, A, j, B, out, false);
            else if (A.shape.type == T::Box && B.shape.type == T::Circle) circleBox(j, B, i, A, out, true);
            else if (A.shape.type == T::Box && B.shape.type == T::Box) boxBox(i, A, j, B, out);
            else if (A.shape.type == T::Circle && B.shape.type == T::Edge) circleEdge(i, A, j, B, out, false);
            else if (A.shape.type == T::Edge && B.shape.type == T::Circle) circleEdge(j, B, i, A, out, true);
            else if (A.shape.type == T::Box && B.shape.type == T::Edge) boxEdge(i, A, j, B, out, false);
            else if (A.shape.type == T::Edge && B.shape.type == T::Box) boxEdge(j, B, i, A, out, true);
        }

        void circleCircle(int i, const BodyData& A, int j, const BodyData& B, std::vector<Contact>& out) {
            Vec2 d = B.pos - A.pos; float dist = d.length();
            float rsum = A.shape.radius + B.shape.radius;
            if (dist >= rsum || dist < 1e-8f) return;
            Vec2 n = d * (1.f / dist);
            out.push_back({ i, j, n, A.pos + n * A.shape.radius, rsum - dist });
        }

        // circle body is (ci,C); box body is (bi,Bx). flip=true means original order was (box, circle) so
        // resulting normal (which we compute as box->circle) must be flipped for a/b = (ci... ) NO -- we keep
        // contact.a = ci (circle owner index passed) consistent with caller's chosen a/b ordering below.
        void circleBox(int ci, const BodyData& C, int bi, const BodyData& Bx, std::vector<Contact>& out, bool flip) {
            Vec2 local = rotate(C.pos - Bx.pos, -Bx.angle);
            Vec2 clamped{ std::clamp(local.x, -Bx.shape.hw, Bx.shape.hw), std::clamp(local.y, -Bx.shape.hh, Bx.shape.hh) };
            Vec2 diffLocal = local - clamped;
            float dist = diffLocal.length();
            bool inside = (dist < 1e-6f);
            Vec2 nLocal;
            float depth;
            if (inside) {
                // circle centre inside box: push out along smallest axis
                float dx = Bx.shape.hw - std::fabs(local.x);
                float dy = Bx.shape.hh - std::fabs(local.y);
                if (dx < dy) { nLocal = { local.x > 0 ? 1.f : -1.f, 0.f }; depth = dx + C.shape.radius; }
                else { nLocal = { 0.f, local.y > 0 ? 1.f : -1.f }; depth = dy + C.shape.radius; }
            }
            else {
                if (dist >= C.shape.radius) return;
                nLocal = diffLocal * (1.f / dist);
                depth = C.shape.radius - dist;
            }
            Vec2 nBoxToCircle = rotate(nLocal, Bx.angle); // points from box -> circle
            Vec2 point = C.pos - nBoxToCircle * C.shape.radius;
            // Contact.normal must point from a -> b.
            if (!flip) out.push_back({ ci, bi, nBoxToCircle * -1.f, point, depth }); // a=circle, b=box
            else       out.push_back({ bi, ci, nBoxToCircle,        point, depth }); // a=box, b=circle
        }

        void circleEdge(int ci, const BodyData& C, int ei, const BodyData& E, std::vector<Contact>& out, bool flip) {
            float distAbove = dot(E.shape.normal, C.pos) - E.shape.offset;
            float depth = C.shape.radius - distAbove;
            if (depth <= 0.f) return;
            Vec2 point = C.pos - E.shape.normal * C.shape.radius;
            if (!flip) out.push_back({ ci, ei, E.shape.normal * -1.f, point, depth }); // a=circle,b=edge, normal a->b
            else       out.push_back({ ei, ci, E.shape.normal, point, depth });        // a=edge,b=circle
        }

        void boxEdge(int bi, const BodyData& Bx, int ei, const BodyData& E, std::vector<Contact>& out, bool flip) {
            auto corners = boxCorners(Bx);
            for (auto& p : corners) {
                float depth = E.shape.offset - dot(E.shape.normal, p);
                if (depth > 0.f) {
                    if (!flip) out.push_back({ bi, ei, E.shape.normal * -1.f, p, depth });
                    else       out.push_back({ ei, bi, E.shape.normal, p, depth });
                }
            }
        }

        void boxBox(int i, const BodyData& A, int j, const BodyData& B, std::vector<Contact>& out) {
            Vec2 axes[4] = { rotate({1,0},A.angle), rotate({0,1},A.angle), rotate({1,0},B.angle), rotate({0,1},B.angle) };
            auto ca = boxCorners(A), cb = boxCorners(B);
            float bestOverlap = 1e30f; Vec2 bestAxis{ 1,0 };
            for (auto& axis : axes) {
                float aMin = 1e30f, aMax = -1e30f, bMin = 1e30f, bMax = -1e30f;
                for (auto& p : ca) { float t = dot(p, axis); aMin = std::min(aMin, t); aMax = std::max(aMax, t); }
                for (auto& p : cb) { float t = dot(p, axis); bMin = std::min(bMin, t); bMax = std::max(bMax, t); }
                float overlap = std::min(aMax, bMax) - std::max(aMin, bMin);
                if (overlap <= 0.f) return; // separating axis found -> no collision
                if (overlap < bestOverlap) { bestOverlap = overlap; bestAxis = axis; }
            }
            Vec2 n = bestAxis;
            if (dot(B.pos - A.pos, n) < 0.f) n = n * -1.f; // make n point A -> B
            // approximate contact point: deepest vertex of B into A (or A into B)
            float worst = 1e30f; Vec2 point = B.pos;
            for (auto& p : cb) { float t = dot(p, n) - dot(A.pos, n); if (t < worst) { worst = t; point = p; } }
            for (auto& p : ca) { float t = -(dot(p, n) - dot(B.pos, n)); if (t < worst) { worst = t; point = p; } }
            out.push_back({ i, j, n, point, bestOverlap });
        }

        void resolveVelocity(const Contact& c) {
            auto& A = bodies_[c.a]; auto& B = bodies_[c.b];
            Vec2 rA = c.point - A.pos, rB = c.point - B.pos;
            Vec2 vA = A.vel + cross(A.angVel, rA);
            Vec2 vB = B.vel + cross(B.angVel, rB);
            Vec2 rv = vB - vA;
            float vn = dot(rv, c.normal);
            if (vn > 0.f) return; // separating

            float rnA = cross(rA, c.normal), rnB = cross(rB, c.normal);
            float invMassSum = A.invMass + B.invMass + rnA * rnA * A.invInertia + rnB * rnB * B.invInertia;
            if (invMassSum <= 0.f) return;

            float e = std::max(A.restitution, B.restitution);
            float jn = -(1.f + e) * vn / invMassSum;
            jn = std::max(jn, 0.f);
            Vec2 impulse = c.normal * jn;
            A.vel -= impulse * A.invMass; A.angVel -= A.invInertia * cross(rA, impulse);
            B.vel += impulse * B.invMass; B.angVel += B.invInertia * cross(rB, impulse);

            // friction
            vA = A.vel + cross(A.angVel, rA); vB = B.vel + cross(B.angVel, rB);
            rv = vB - vA;
            Vec2 tangent = rv - c.normal * dot(rv, c.normal);
            float tl = tangent.length();
            if (tl > 1e-6f) {
                tangent = tangent * (1.f / tl);
                float rtA = cross(rA, tangent), rtB = cross(rB, tangent);
                float invMassSumT = A.invMass + B.invMass + rtA * rtA * A.invInertia + rtB * rtB * B.invInertia;
                if (invMassSumT > 0.f) {
                    float jt = -dot(rv, tangent) / invMassSumT;
                    float mu = 0.5f * (A.friction + B.friction);
                    jt = std::clamp(jt, -mu * jn, mu * jn);
                    Vec2 fImpulse = tangent * jt;
                    A.vel -= fImpulse * A.invMass; A.angVel -= A.invInertia * cross(rA, fImpulse);
                    B.vel += fImpulse * B.invMass; B.angVel += B.invInertia * cross(rB, fImpulse);
                }
            }
        }

        void positionalCorrection(const Contact& c) {
            auto& A = bodies_[c.a]; auto& B = bodies_[c.b];
            const float percent = 0.4f, slop = 0.005f;
            float invMassSum = A.invMass + B.invMass;
            if (invMassSum <= 0.f) return;
            float corrMag = std::max(c.depth - slop, 0.f) / invMassSum * percent;
            Vec2 corr = c.normal * corrMag;
            if (!A.isStatic) A.pos -= corr * A.invMass;
            if (!B.isStatic) B.pos += corr * B.invMass;
        }
    };

} // namespace mini2d