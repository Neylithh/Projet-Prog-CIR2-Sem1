#include "avion.hpp"

Position::Position(double x, double y, double z) : x_(x), y_(y), altitude_(z) {}
double Position::getX() const { return x_; }
double Position::getY() const { return y_; }
double Position::getAltitude() const { return altitude_; }
void Position::setPosition(double x, double y, double alt) { x_ = x; y_ = y; altitude_ = alt; }
double Position::distance(const Position& other) const {
    double dx = x_ - other.x_;
    double dy = y_ - other.y_;
    double dz = altitude_ - other.altitude_;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
