#pragma once
#ifndef OBJECTS_H
#define OBJECTS_H

#include <algorithm>
#include <random>
#include "geometry.hpp"
#include "image.hpp"
#include "utils.hpp"

//===============================================================//
// Properties
//===============================================================//
class Box_collider {
private:
    //...
public:
    std::vector<Vector3> b;
    Vector3 center;

    Box_collider() : b({Vector3(INFINITY, INFINITY, INFINITY), Vector3(-INFINITY, -INFINITY,-INFINITY)}) {}
    Box_collider(Vector3 min, Vector3 max) : b({ min, max }), center((max-min)/2) {}

    bool intersects(const Ray& r) {

        Vector3 inv_d(1.f/r.d.x, 1.f/r.d.y, 1.f/r.d.z);
        bool sign_dir_x = inv_d.x < 0;
        bool sign_dir_y = inv_d.y < 0;
        bool sign_dir_z = inv_d.z < 0;

        double p0 = (b[sign_dir_x].x - r.p.x) * inv_d.x;
        double p1 = (b[1 - sign_dir_x].x - r.p.x) * inv_d.x;
        double b0 = (b[sign_dir_y].y - r.p.y) * inv_d.y;
        double b1 = (b[1 - sign_dir_y].y - r.p.y) * inv_d.y;
        if ((p0 > b1) || (b0 > p1)) return false;
        if (b0 > p0) p0 = b0;
        if (b1 < p1) p1 = b1;

        b0 = (b[sign_dir_z].z - r.p.z) * inv_d.z;
        b1 = (b[1 - sign_dir_z].z - r.p.z) * inv_d.z;
        if ((p0 > b1) || (b0 > p1)) return false;
        if (b0 > p0) p0 = b0;
        if (b1 < p1) p1 = b1;

        return (p0 < 0 && p1 < 0) ? false : true;

    }

    void print(std::ostream& os, int i) {
        std::string s = bleeding("  ", i);
        os << s + "BOX COLLIDER {"
            << "\n" + s + "  bounds {" << b[0] << ", " << b[1] << " }"
            << "\n" + s + "  center: " << center
            << "\n" + s + "}\n";
    }

};

class Object;

struct Collision {
    std::shared_ptr<Object> obj; // Collisioned object.
    Vector3 normal;              // Collisioned object normal.
    Vector3 point;               // Collision point.
    double dist;                 // Collision distance.

    Vector3 wo;
    Vector3 wi;              // To store the bounce's directions

    Collision()
        : normal(Vector3()), point(Vector3()), dist(INFINITY) {}
    Collision(double d)
        : normal(Vector3()), point(Vector3()), dist(d) {}
    Collision(Vector3 n, Vector3 p, double d)
        : normal(n), point(p), dist(d) {}

};

//===============================================================//
// UV Coordinates
//===============================================================//
using VectorUV = Vector2;

struct Bounds {
    // Spheres
    Vector3 c;
    double r = 0;  // Stored for performance
    // Spheres and Unbounded planes
    Vector3 normal;
    // Planes
    double D = 0.0;
    // Bounded planes
    std::vector<Vector3> vP;    // Vertice positions
    std::vector<Vector3> vN;    // Vertice normals
    std::vector<VectorUV> vUV;  // Vertice UV coordinates

    // Default
    Bounds() {}
    // Spheres
    Bounds(Vector3 c, Vector3 axis) : c(c), r(axis.mod()), normal(nor(axis)) {}
    // Unbounded planes
    Bounds(Vector3 normal, double D) : normal(normal), D(D) {}
    // Bounded planes
    Bounds(std::vector<Vector3> vP) : vP(vP) {}
    Bounds(std::vector<Vector3> vP, std::vector<Vector3> vN) : vP(vP), vN(vN) {}
    Bounds(std::vector<Vector3> vP, std::vector<VectorUV> vUV) : vP(vP), vUV(vUV) {}
    Bounds(std::vector<Vector3> vP, std::vector<Vector3> vN, std::vector<VectorUV> vUV) : vP(vP), vN(vN), vUV(vUV) {}
};

VectorUV polyPoint_2_UV(Bounds& b, const Vector3& n, const Vector3& p) {
    VectorUV result;
    // if sphere (radius > 0) note: radius is by default 0
    if (b.r > EPSILON_ERROR) {
        double phi = atan2(p.y - b.c.y, p.x - b.c.x);
        double theta = acos((p.z - b.c.z) / b.r);

        result.x = b.r * (phi / 2*M_PI);
        result.y = b.r * ((M_PI - theta) / M_PI);
    } else {
        std::vector<double> barycentric(b.vP.size());

        // barycentric[i] = (dot(N, V[i]->P)) / sum (dot(N, V[0]->V[i]))
        double denom = 0;
        for (unsigned int i = 1; i < b.vP.size(); i++) {
            denom += n * (b.vP[i] - b.vP[0]);
        }
        for (unsigned int i = 0; i < barycentric.size(); i++) {
            barycentric[i] = (n * (p - b.vP[i])) / denom;
        }

        if (b.vUV.empty()) {
            if (b.vP.size() == 3) {
                b.vUV.push_back(VectorUV(0.0,1.0));
                b.vUV.push_back(VectorUV(1.0,1.0));
                b.vUV.push_back(VectorUV(0.0,0.0));
            }
            else if (b.vP.size() == 4) {
                b.vUV.push_back(VectorUV(0.0,1.0));
                b.vUV.push_back(VectorUV(1.0,1.0));
                b.vUV.push_back(VectorUV(1.0,0.0));
                b.vUV.push_back(VectorUV(0.0,0.0));
            }
        }
        
        debug(b.vUV.size() != b.vP.size(), "El objeto no tiene tantas coordenadas UV como vértices");

        //UV_P = sum(barycentric[i] * UV[i])
        for (unsigned int i = 0; i < barycentric.size(); i++) {
            result += barycentric[i] * b.vUV[i];
        }
    }
    return result;
}

//===============================================================//
// Textures
//===============================================================//
class Texture {
private:
public:
    Image i;
    Texture(std::string path) : i(path) {}
    RGB getValue (VectorUV uv) const
    {
        int indexu = uint32_t(uv.x * i.width) % i.width;
        int indexv = uint32_t(uv.y * i.height) % i.height;
        RGB color = i.pixels[indexu][indexv];
        return color;
    }
    RGB getValue (Bounds& b, const Vector3& n, const Vector3& p) const {
        return getValue(polyPoint_2_UV(b,n,p));
    }
};

//===============================================================//
// Materials
//===============================================================//

struct Sample {

    Vector3 wi;   // Resultant direction.
    RGB fr;       // Color factor.

    // In some materials getting the direct light doesn't make sense with the
    // way the scattering is simulated. For example:
    //
    // With specularity, we redirect the direction based in the Snell's theorem,
    // the probability that the resultant direction intersects with a point 
    // light is practically 0 as we redirect in purpose the direction (yes,
    // could be a light right on the direction trayectory, but the chance is 
    // too small to take in account).
    //
    // Then we indicate if the d_light has to be calculated or not.
    bool is_delta;

    Sample()
        : wi(Vector3()), fr(RGB()), is_delta(true) {}
    Sample(Vector3 wi, RGB fr, bool is_delta)
        : wi(wi), fr(fr), is_delta(is_delta) {}
};

class BXDF {
private:
    bool has_tex;
    std::shared_ptr<Texture> tex;
    RGB val;
public:
    double p;
public:
    BXDF() {}
    BXDF(RGB k) : has_tex(false), val(k), p(max(k)) {}
    BXDF(std::shared_ptr<Texture> t, double p = 0.9) : has_tex(true), tex(t), val(RGB()), p(p) {}

    // RGB operator() () const {
    //     return val;
    // }

    RGB operator() (Bounds& b, const Vector3& n, const Vector3& p, const Vector3& wi, const Vector3& wo) const {

        if (has_tex) return tex->getValue(b,n,p);
        else return val;
    }

    std::ostream& print(std::ostream& os) const {
        if (has_tex) return os << "texture: " + tex->i.name;
        else return os << val << ", " << p*100 << "%";
    }
};

std::ostream& operator<<(std::ostream& os, const BXDF& bxdf) {
    return bxdf.print(os);
}

class Material {
private:

    // First version before Fresnell.
    void coefficient_correction() {

        // coefficients have been initialized on constructor

        double coeff = kd.p + ks.p + kt.p;
        if (coeff > 1) {
            kd.p /= coeff;
            ks.p /= coeff;
            kt.p /= coeff;
        }
    }

    struct Fresnel {
        Vector3 n;
        Vector3 wo;
        double ref_coef;
        double local_ps;
        double local_pt;
        double cos_i;
        double cos_t2;
    };

    // etai = n_1, etat = n_2. ref_coef = n_1/n_2.
    // etat = ref_index_i, etai = ref_index_o.
    Fresnel fresnel_coefficients(Vector3 n, Vector3 wo, double ref_index_o, double ref_index_i) {
        double fresnel_ps = ks.p, fresnel_pt = kt.p;
        if (kt.p > 0) {
            if ((n * wo) > 0) {
                n *= -1;
                std::swap(ref_index_o, ref_index_i);
            }
            wo = nor(wo);
            double ref_coef = ref_index_o/ref_index_i;
            double cos_i = n * wo;
            double cos_t2 = 1.0 - ref_coef * ref_coef * (1 - cos_i * cos_i);
            if (ks.p > 0) {
                if (cos_t2 < 0) {
                    fresnel_ps = 1;
                    fresnel_pt = 0;
                } else {
                    double cos_t = sqrt(cos_t2);
                    double ncos_i = fabsf(cos_i);
                    double Rs = ((ref_index_i * ncos_i) - (ref_index_o * cos_t))
                        / ((ref_index_i * ncos_i) + (ref_index_o * cos_t));
                    double Rp = ((ref_index_o * ncos_i) - (ref_index_i * cos_t))
                        / ((ref_index_o * ncos_i) + (ref_index_i * cos_t));
                    fresnel_ps = (Rs * Rs + Rp * Rp)/2.0;
                    fresnel_pt = 1 - fresnel_ps;
                }
                fresnel_ps *= (ks.p + kt.p);
                fresnel_pt *= (ks.p + kt.p);
            }
            return {n, wo, ref_coef, fresnel_ps, fresnel_pt, cos_i, cos_t2};
        }
        return {Vector3(), Vector3(), 0, fresnel_ps, fresnel_pt, 0, 0};
    }

public:
    BXDF kd, ks, kt;
    // Material emission.
    RGB ke;

    // Material refractance index.
    double ref_index_i;

    Material(BXDF kd = BXDF(RGB(185,185,185)), BXDF ks = BXDF(RGB()), BXDF kt = BXDF(RGB()),
        RGB ke = RGB(), double ref_index_i = 0)
    {
        // Lambertian diffuse parameters.
        this->kd = kd;
        // Perfect specular reflectance parameters.
        this->ks = ks;
        // Perfect refrectation parameters.
        this->kt = kt;
        // Material emission.
        this->ke = ke;

        // Material refraction index.
        this->ref_index_i = ref_index_i;

        // Coefficients correction.
        coefficient_correction();

    }

    Sample scattering(Bounds& b, Vector3 n, Vector3 p, Vector3 wo, double ref_index_o = 1) {

        // Russian roulette event generator.
        double rr_event = E(e2);

        // Fresnel coefficients evaluation:
        Fresnel fr = fresnel_coefficients(n, wo, ref_index_o, ref_index_i);

        // Lambertian diffuse event:
        if (kd.p > 0 && rr_event < kd.p) {
            double lat = acos(sqrt(1 - E(e2))); // SE GENERAN COMO RADIANES
            double azi = 2*M_PI*E(e2);          // LO HE COMPROBADO.
            // Orthonormal basis:
            std::vector<Vector3> basis = orthonormal_basis(n);
            // New direction sampling:
            Vector3 dir = Matrix3BaseChange(basis[0], basis[1], n, Vector3())
                * Vector3(sin(lat)*cos(azi), sin(lat)*sin(azi), cos(lat));
            return Sample(dir, kd(b,n,p,dir,wo)/kd.p, false);
        }
        // Perfect specular reflectance event:
        else if (fr.local_ps > 0 && rr_event < (kd.p + fr.local_ps)) {
            Vector3 dir = wo - ((2*n)*(wo*n));
            return Sample(dir, ks(b,n,p,dir,wo)/fr.local_ps, true);
        }
        // Perfect refrectation event:
        // - https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
        // - https://stackoverflow.com/a/58676386
        else if (fr.local_pt > 0 && rr_event < (kd.p + fr.local_ps + fr.local_pt)) {
            if (ks.p == 0 && fr.cos_t2 < 0) {
                Vector3 dir = wo - ((2*n)*(wo*n));
                return Sample(dir, kt(b,n,p,dir,wo)/fr.local_pt, true);
            } else {
                Vector3 dir = fr.ref_coef * (fr.wo - fr.n * fr.cos_i) - fr.n * sqrtf(fr.cos_t2);
                return Sample(dir, kt(b,n,p,dir,wo)/fr.local_pt, true);
            }
        }
        // Ray death event:
        else {
            return Sample();
        }
    }

    void print(std::ostream& os, int i) {
        std::string s = bleeding("  ", i);
        os << s + "MATERIAL {"
            << "\n" + s + "  Diffuse: " << kd
            << "\n" + s + "  Specular: " << ks
            << "\n" + s + "  Refraction: " << kt << ", coef: " << ref_index_i
            << "\n" + s + "  Emission: " << ke
            << "\n" + s + "}\n";
    }

};

//===============================================================//
// Light photon
//===============================================================//
class Photon {
private:
    // ... 
public:
    // Photon position.
    Vector3 pos;
    // Photon flux.
    RGB flux;
    // Photon next direction.
    Vector3 wi;
    Photon() {}
    Photon(Vector3 pos, RGB flux, Vector3 wi) : pos(pos), flux(flux), wi(wi) {}
};

std::ostream& operator<<(std::ostream& os, const Photon& p) {
    return os << "Photon { " << p.pos << ", " << p.flux << ", " << p.wi << " }";
}

void operator>>(std::istream& in, Photon& p) {
    in >> p.pos;
    in >> p.flux;
    in >> p.wi;
}

struct PhotonAxisPosition {
    double operator()(const Photon& p, std::size_t i) const {
        return p.pos[i];
    }
};

//===============================================================//
// Point light
//===============================================================//
class Light {
private:
    // ...
public:
    // Point light center.
    Vector3 c;
    // Point light power.
    RGB pow;

    Light(Vector3 c = Vector3(), RGB pow = RGB()) : c(c), pow(pow) {}
};

//=================================================================//
// Objects
//=================================================================//

class Object {
private:
    // ...
public:

    // Object info.
    Bounds b;   // Data that defines what is inside the Object and what's outside of it
    Material m; // Object material.
    Box_collider collider; // Object box collider.

    Object(Material m = Material()) : m(m) {} 
    Object(Bounds b, Material mat = Material()) : b(b), m(mat) {}

    virtual ~Object() = default;

    virtual Collision intersects(const Ray& ray) = 0;
    // Print object properties.
    virtual void print(std::ostream& os, int i) = 0;
};

//===============================================================//
// Plane
//===============================================================//

class Plane : public Object {
private:
    // ...
public:
    // Geometrical properties.
    double D;               // Implicit equation A*x + B*y + C*z + D
                            //    (= 0 if the point is in the plane).
    Vector3 n;              // Normal of the plane = (A,B,C).
    std::vector<Vector3> b; // Vertex bounds of the plane (if finite).

    // Default constructor.
    Plane() {}

    // ==========================
    // Plane constructors
    // ==========================

    // Solid color plane defined by a normal and its distance to the origin.
    Plane(double D, Vector3 n, Material m = Material())
        : Object(m), D(D), n(nor(n)), b({}) {}

    // Plane defined by a normal and a plane contained point with solid color.
    Plane(Vector3 p, Vector3 n, Material m = Material()) : Object(m) {
        this->n = n;
        this->D = -p * this->n;
        this->b = {};
    }

    // ==========================
    // Finite plane constructors
    // ==========================
    //Plane(double D, std::vector<Vector3> b, Material m = Material())
    //    : Object(m), D(D), n(nor(crs(b[1]-b[0], b[3]-b[0]))), b(b) {}

    Plane(std::vector<Vector3> b, Material m = Material()) : Object(m) {
        this->n = nor(crs(b[1]-b[0], b[2]-b[0]));
        this->D = -b[0] * n;
        this->b = {};
    }

    // B +----+ C
    //   |  / |
    //   | /  |
    // A +----+ D
    Collision intersects(const Ray& r) override {

        // Si la division es 0 no hay corte.
        auto fix = r.p + (n * 0.00001);
        if (std::abs(n*r.d) < 0) return Collision(-1);

        // Calcula la distancia desde el centro del rayo hasta el punto de corte.
        double t = -(n*fix + D)/(n*r.d);                                
        if (t < 0) return Collision(-1);

        // Extra check for all edges if bounded
        Vector3 x = (r.d * t + fix), usn = (n * r.d < 0) ? n : -n;
        for (uint32_t i = 0; i < b.size(); i++) {
            if (usn * crs(b[(i+1) % b.size()] - b[i], x - b[i]) < 0) {
                return Collision(-1);
            }
        }

        // Devolver la normal correcta junto con la distancia al punto de corte.
        return Collision(usn, x, t);
    }

    // Print plane properties.
    void print(std::ostream& os, int i) override {
        std::string s = bleeding("  ", i);
        os << s + "PLANE {"
            << "\n" + s + "  normal: " << n 
            << "\n" + s + "  distance: " << D
            << "\n" + s + "  finite plane bounds {";
        if (b.size()) {
            for (auto& v : b) {
                os << " " << v;
            }
            os << " }\n";
        } else {
            os << "}\n";
        }
        m.print(os, i+1);
        os << s + "}\n";
    }

};

//===============================================================//
// Triangle
//===============================================================//
class Triangle : public Plane {
private:
    // ...
public:
    // Default constructor.
    Triangle() {}
    // Triangle boundings.
    Triangle(std::vector<Vector3> b, Material m = Material()) : Plane(b, m) {}

    Collision intersects(const Ray& r) override {
        // Plane::intersects performs an n-edge check also valid for Triangle
        // implementation:
        return Plane::intersects(r);
    }

    // Print triangle properties.
    void print(std::ostream& os, int i) override {
        std::string s = bleeding("  ", i);
        os << s + "TRIANGLE {"
            << "\n" + s + "  normal: " << n
            << "\n" + s + "  triangle bounds {";
        if (b.size()) {
            for (auto& v : b) {
                os << "\n" + s + "    " << v;
            }
            os << "\n" + s + "  }\n";
        }
        else {
            os << "}\n";
        }
        m.print(os, i+1);
        os << s + "}\n";
    }

};

class Circle : public Plane {
private:
    //...
public:
    Vector3 center;
    double radius;

    Circle() : Plane(Vector3(), Vector3(), Material()) {}
    Circle(Vector3 center, Vector3 normal, double radius, Material m = Material())
        : Plane(center, normal, m), center(center), radius(radius) {}

    Collision intersects(const Ray& r) override {
        auto xc = Plane::intersects(r);
        if (xc.dist < 0) return Collision(-1);
        if ((xc.point - center).mod() > radius) return Collision(-1);
        return xc;
    }

    // Print triangle properties.
    void print(std::ostream& os, int i) override {
        std::string s = bleeding("  ", i);
        os << s + "CIRCLE {" << s + "}\n";
    }

};

//===============================================================//
// Cone (NOT DEBUGGED)
//===============================================================//
class Cone : public Object {
private:
    // An expresion that delimits the intersections inside the cone angle.
    double ang_restrict; 
public:   
    Vector3 C; // Cone center.
    Vector3 H; // Cone axis.
    Vector3 h;
    double angle;
    double radius;  // Cone radius.
    double height;
    double m;
    Circle bottom;

    Cone(Vector3 C, Vector3 H, double radius, Material m = Material()) : Object(m) {
        this->C = C;
        this->H = H;
        this->h = nor(C-H);
        this->radius = radius;
        this->height = (C-H).mod();
        this->angle  = std::atan(radius/height);
        this->m = (radius*radius)/(height*height);
        bottom = Circle(C, h, radius, Material(RGB(0,0,180)));
    }

    Collision intersects(const Ray& r) override {
        /*
        double A = r.p.x - C.x;
        double B = r.p.z - C.z;
        double D = height - r.p.y + C.y;
        double tan = (radius/height)*(radius/height);

        float a = (r.d.x*r.d.x) + (r.d.z*r.d.z) - (tan*(r.d.y*r.d.y));
        float b = (2*A*r.d.x) + (2*B*r.d.z) + (2*tan*D*r.d.y);
        float d = A*A + B*B - tan*D*D;

        float discr = b*b - 4*a*d;
        if(discr < EPSILON_ERROR) return Collision(-1);
        double t1 = (-b - sqrt(discr))/(2*a);
        double t2 = (-b + sqrt(discr))/(2*a);
        double t = t1 < t2? t1: t2;

        Vector3 x = r.p + t*r.d;
        // If it doesn't hit cylinder walls
        if(x.y < C.y || x.y > (C.y + height)) {
            // Cannot intersect with cap because ray is parallel:
            if(Vector3(0,-1,0) * r.d < EPSILON_ERROR) return Collision(-1);

            float t = ((C - r.p) * Vector3(0,-1,0)) / (Vector3(0,-1,0)*r.d);
            x = r.p + t * r.d;
            auto tmp = C - x;
            if(tmp*tmp > radius*radius){
                return Collision(-1);
            }
            return Collision(Vector3(0,-1,0), x, t);

        } else {
            Vector3 n = x - C;
            n.y = (n - Vector3(0,n.y,0)).mod() * (radius/height);
            return Collision(nor(n), x, t);
        }*/

        // Calculating if the cone was intersected:
        Vector3 L = r.p - H;
        double a = (r.d*r.d) - (m+1)*((r.d*h)*(r.d*h));
        double b = 2*((r.d*L) - (m+1)*((r.d*h)*(L*h)));
        double c = (L*L) - (m+1)*((L*h)*(L*h));

        double discr = b*b - (4*a*c), t;
        if (discr < 0) return Collision(-1);
        else if (discr == 0) {
            if((r.d * h) == (height/sqrt(height*height + radius*radius))) {
                return Collision(-1);
            } else {
                t = -b/(2*a);
            }
        } else {
            double t0 = (-b - sqrt(discr))/(2*a);
            double t1 = (-b + sqrt(discr))/(2*a);
            t = t0 < t1 ? t0 : t1;
        }

        // Calculating if the bottom cap was intersected:
        //auto prev_c = bottom.intersects(r);
        //if (prev_c.dist > 0 && prev_c.dist <= t) {
        //    prev_c.obj = std::make_shared<Circle>(bottom);
        //    return prev_c;
        //}

        Vector3 x = r.p + t*r.d;
        if (0 <= (x-H)*h && (x-H)*h <= height) {
            Vector3 R = nor(x-H, sqrt(height*height + radius*radius));
            Vector3 n = nor(crs(x-H,x-R));
            return Collision((r.d*n < 0) ? n : -n, x, t);
        }
        return Collision(-1);
        //if ((x-H)*h > 0 && (x-C)*h < 0) {
        //    Vector3 R = nor(x-H, sqrt(height*height + radius*radius));
        //    Vector3 n = nor(crs(x-H,x-R));
        //    return Collision((r.d*n < 0) ? n : -n, x, t);
        //} else {
        //    return Collision(-1);
        //}
    }

    // Print triangle properties.
    void print(std::ostream& os, int i) override {
        std::string s = bleeding("  ", i);
        os << s + "CONE {" << C-H << ", " << h << "}\n";
    }

};

//===============================================================//
// Cylinder (NOT DEBUGGED)
//===============================================================//
class Cylinder : public Object {
private:
    // ...
public:
    Vector3 center; // Cylinder center.
    Vector3 axis;   // Cylinder axis (thus height and orientation).
    double radius;  // Cylinder base radius.

    Cylinder(Vector3 center, Vector3 axis, double radius)
        : center(center), axis(axis), radius(radius) {}

    Collision intersects(const Ray& r) override {
        // Vector from cylinder center to ray origin.
        Vector3 L = r.p - center;
        double t_ca = L * axis;
        double d_ca = r.d * axis;
        Vector3 p_ca = axis * t_ca;
        Vector3 V = L - p_ca;
        double v_length_sq = V*V;
        double d_sq = d_ca * d_ca;
        double discr = (radius * radius) * d_sq - v_length_sq * d_ca * d_ca;
        if (discr < 0) return Collision(-1); 

        double t1 = (t_ca + sqrt(discr)) / d_sq;
        double t2 = (t_ca - sqrt(discr)) / d_sq;
        if (t1 <= EPSILON_ERROR && t2 <= EPSILON_ERROR) return Collision(-1);
        if (t1 > EPSILON_ERROR) {
            Vector3 x = r.d * t1 + r.p;
            Vector3 x_axis = center + (axis * ((x - center) * axis));
            return Collision(nor(x - x_axis), x, t1);
        } else {
            Vector3 x = r.d * t2 + r.p;
            Vector3 x_axis = center + (axis * ((x - center) * axis));
            return Collision(nor(x - x_axis), x, t2);
        }
    }
};

//===============================================================//
// Sphere
//===============================================================//

class Sphere : public Object {
private:
    //...
public:
    // Geometrical properties:
    double  radius; // Sphere radius.
    Vector3 center; // Sphere center.
    Vector3 axis;   // Sphere axis.

    Sphere(double radius, Vector3 center, Material m = Material())
        : Object(m), radius(radius), center(center) {}
    Sphere(Vector3 center, Vector3 axis, Material m = Material())
        : Object(m), radius(axis.mod()/2), center(center), axis(axis) {}

    Collision intersects(const Ray& r) override {

        Vector3 L = r.p - b.c;
        double A = r.d * r.d;
        double B = 2 * r.d * L;
        double C = L * L - b.r * b.r;
        double delta = B*B - 4*A*C;

        if (delta < EPSILON_ERROR) return Collision(-1);
        double t0 = (-B - sqrt(delta)) / (2*A);
        double t1 = (-B + sqrt(delta)) / (2*A);
        if (t0 <= EPSILON_ERROR && t1 <= EPSILON_ERROR) return Collision(-1);

        if (t0 > EPSILON_ERROR) {
            Vector3 x = r.d * t0 + r.p;
            return Collision(nor(x-b.c), x, t0);
        } else {
            Vector3 x = r.d * t1 + r.p;
            return Collision(nor(x-b.c), x, t1);
        }
    }

    // Print sphere properties.
    void print(std::ostream& os, int i) override {
        std::string s = bleeding("  ", i);
        os << s + "SPHERE {"
            << "\n" + s + "  center: " << center
            << "\n" + s + "  axis: "   << axis
            << "\n" + s + "  radius: " << radius << "\n";
        m.print(os, i+1);
        os << s + "}\n";
    }

};

//===============================================================//
// Cube
//===============================================================//

class Cube : public Object {
private:
    // ...
public:

    Vector3 center;
    std::vector<Vector3> b;
    
    // Cube bounds representation:
    //        b[5]             max (b[6])
    //            +-----------+
    //           /.          /|
    //     b[1] +-----------+<--- b[2]
    //          | .         | |
    //     b[4]-| ..FRONT...|.+ b[7]
    //          |.          |/
    //          +-----------+
    //     min (b[0])         b[3]
    std::vector<Plane> faces; // Cube faces.

    // Default constructor.
    Cube() {}
    // Cube constructor with the minimum and maximum bounds.
    Cube(Vector3 min, Vector3 max, Material m = Material()) : Object(m) {

        // Vertex order:
        b.push_back(min);                          // b[0]: -x, -y, -z.
        b.push_back(Vector3(min.x, max.y, min.z)); // b[1]: -x, +y, -z.
        b.push_back(Vector3(max.x, max.y, min.z)); // b[2]: +x, +y, -z.
        b.push_back(Vector3(max.x, min.y, min.z)); // b[3]: +x, -y, -z.
        b.push_back(Vector3(min.x, min.y, max.z)); // b[4]: -x, -y, +z.
        b.push_back(Vector3(min.x, max.y, max.z)); // b[5]: -x, +y, +z.
        b.push_back(max);                          // b[6]: +x, +y, +z.
        b.push_back(Vector3(max.x, min.y, max.z)); // b[7]: +x, -y, +z.
        // Faces order:
        faces.push_back(Plane({b[0],b[1],b[2],b[3]}, m)); // Front face.
        faces.push_back(Plane({b[3],b[2],b[6],b[7]}, m)); // Right face.
        faces.push_back(Plane({b[7],b[6],b[5],b[4]}, m)); // Back face.
        faces.push_back(Plane({b[4],b[5],b[1],b[0]}, m)); // Left face.
        faces.push_back(Plane({b[1],b[5],b[6],b[2]}, m)); // Top face.
        faces.push_back(Plane({b[0],b[3],b[7],b[4]}, m)); // Bottom face.

    }

    // Cube constructor with each vertice.
    Cube(std::vector<Vector3> b, Material m = Material()) : Object(m) {
        
        // Vertex order:
        this->b = b;
        // Cube faces:
        faces.push_back(Plane({b[0],b[1],b[2],b[3]}, m)); // Front face.
        faces.push_back(Plane({b[3],b[2],b[6],b[7]}, m)); // Right face.
        faces.push_back(Plane({b[7],b[6],b[5],b[4]}, m)); // Back face.
        faces.push_back(Plane({b[4],b[5],b[1],b[0]}, m)); // Left face.
        faces.push_back(Plane({b[1],b[5],b[6],b[2]}, m)); // Top face.
        faces.push_back(Plane({b[0],b[3],b[7],b[4]}, m)); // Bottom face.

    }

    // Cube constructor with each plane.
    Cube(std::vector<Plane> faces) : Object(m), faces(faces) {

        // We might still want the vertices of our cube, so we will also extract them
        debug(faces.size() != 6, "This is not a cube, what is this?");

        // Following our cube bounds representation:
        // Front face
        b.push_back(faces[0].b[0]);
        b.push_back(faces[0].b[1]);
        b.push_back(faces[0].b[2]);
        b.push_back(faces[0].b[3]);
        
        // Back face
        b.push_back(faces[2].b[3]);
        b.push_back(faces[2].b[2]);
        b.push_back(faces[2].b[1]);
        b.push_back(faces[2].b[0]);
    }

    // Cube constructor with each plane.
    Cube(std::vector<Plane> faces, Material m) : Object(m), faces(faces) {

        // Change the faces' materials to that of our cube
        for (auto& f : faces) f.m = m;

        // We might still want the vertices of our cube, so we will also extract them
        debug(faces.size() != 6, "This is not a cube, what is this?");

        // Following our cube bounds representation:
        // Front face
        b.push_back(faces[0].b[0]);
        b.push_back(faces[0].b[1]);
        b.push_back(faces[0].b[2]);
        b.push_back(faces[0].b[3]);
        
        // Back face
        b.push_back(faces[2].b[3]);
        b.push_back(faces[2].b[2]);
        b.push_back(faces[2].b[1]);
        b.push_back(faces[2].b[0]);
    }

    Collision intersects(const Ray& r) override {

        Collision c;
        for (auto& face : faces) {
            auto t = face.intersects(r);
            if (t.dist > 0 && t.dist < c.dist) {
                c = t;
                c.obj = std::make_shared<Plane>(face);
            }
        }
        if (c.dist > 0 && c.dist != INFINITY) {
            return c;
        } else {
            return Collision(-1);
        }
    }

    // Print cube properties.
    void print(std::ostream& os, int i) override {
        std::string s = bleeding("  ", i);
        // os << s + "CUBE {"
        //     << "\n" + s + "  cube bounds {";
        // if (b.size()) {
        //     for (auto& v : b) {
        //         os << "\n" + s + "    " << v;
        //     }
        //     os << "\n" + s + "  }";
        // } else {
        //     os << "}";
        // }
        os << "\n" + s + "  faces {\n"; 
        if(faces.size()){
           for (auto& f : faces) {
               f.print(os, i+2);
           }
           os << s + "  }\n";
        } else {
           os << s + "}\n";
        }
        m.print(os, i+1);
        os << s + "}\n";
    }

};

//===============================================================//
// Meshes and its faces: triangles.
//===============================================================//

class Mesh : public Object {
private:
    // Ply metadata: nothing for the moment.
    struct Ply_Vertex {
        Vector3 pos;
        Vector3 n;
        VectorUV uv;
    };
public:
    std::vector<std::vector<Triangle>> parts;
    std::vector<Triangle> faces; // Mesh faces.

    Mesh(std::vector<Triangle> faces, Material m) : Object(m), faces(faces)
    {   
        for (auto& f : faces) {
            f.m = m;
            for (auto& v : f.b) {
                collider.b[0] = min(collider.b[0], v);
                collider.b[1] = max(collider.b[1], v);
            }
        }
    }

    Mesh(std::string ply_file, Material m = Material()) : Object(m) {

        // Reading the PLY file:
        std::string s("");
        std::ifstream in(ply_file);
        debug(!in.is_open(), "file '" + ply_file + "' not found, check it out!");

        std::vector<Ply_Vertex> vertex;
        // Reading the PLY header:
        while (s.compare("end_header")) {
            s = get_line(in);
            if (s.find("element vertex") != std::string::npos) {
                vertex = std::vector<Ply_Vertex>(
                    std::stoi(replace(s, int_d))
                );
            } else if (s.find("element face") != std::string::npos) {
                faces  = std::vector<Triangle>(
                    std::stoi(replace(s, int_d))
                );
            }
        }
        debug(vertex.empty() && faces.empty(), "no vertex..? no faces..?"); // no bitches..? Sorry

        // Reading the PLY vertex list:
        for (auto& v : vertex) {
            in >> v.pos;
            // Defining the mesh collider.
            collider.b[0] = min(collider.b[0], v.pos);
            collider.b[1] = max(collider.b[1], v.pos);
        }

        // Reading the PLY faces list.
        int n, v1, v2, v3;
        for (auto& f : faces) {
            in >> n >> v1 >> v2 >> v3;
            f = Triangle({vertex[v1].pos, vertex[v2].pos, vertex[v3].pos}, m);
        }
    }

    Collision intersects(const Ray& r) override {
        // If the ray doesn't intersects the mesh collider, then it won't
        // intersect anything, just return no collision.
        if (!collider.intersects(r)) return Collision(-1);

        // Mesh internal collision.
        Collision c;
        // Calculating possible intersections with the mesh faces:
        for (auto& f : faces) {
            auto t = f.intersects(r);
            if (t.dist > 0 && t.dist < c.dist) {
                c = t;
                c.obj = std::make_shared<Triangle>(f);
            }
        }
        return (c.dist != INFINITY) ? c : Collision(-1);
    }

    // Print mesh properties.
    void print(std::ostream& os, int i) override {
        std::string s = bleeding("  ", i);
        os << s + "MESH {\n";
        collider.print(os, i+1);
        os << s + "  FACES {\n"; 
        if(faces.size()){
            for (auto& f : faces) {
                f.print(os, i+2);
            }
            os << s + "  }\n";
        } else {
            os << s + "}\n";
        }
        m.print(os, i+1);
        os << s + "}\n";
    }

};

Mesh operator*(Mesh mesh, Matrix3 transform) {

    mesh.collider = Box_collider();
    for (auto& f : mesh.faces) {
        for (auto& v : f.b) {
            v.h = 1;
            v = transform * v;
            mesh.collider.b[0] = min(mesh.collider.b[0], v);
            mesh.collider.b[1] = max(mesh.collider.b[1], v);
        }
        f = Triangle(f.b, f.m);
    }
    return mesh;

};

#endif