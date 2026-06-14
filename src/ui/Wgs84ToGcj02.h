#include <cmath>

struct Coord {
    double lat;
    double lng;
};

static constexpr double PI = 3.14159265358979323846;
static constexpr double A = 6378245.0;
static constexpr double EE = 0.00669342162296594323;

static bool outOfChina(double lat, double lng)
{
    return lng < 72.004 || lng > 137.8347 ||
           lat < 0.8293 || lat > 55.8271;
}

static double transformLat(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y
                 + 0.1 * x * y + 0.2 * std::sqrt(std::abs(x));

    ret += (20.0 * std::sin(6.0 * x * PI)
            + 20.0 * std::sin(2.0 * x * PI)) * 2.0 / 3.0;

    ret += (20.0 * std::sin(y * PI)
            + 40.0 * std::sin(y / 3.0 * PI)) * 2.0 / 3.0;

    ret += (160.0 * std::sin(y / 12.0 * PI)
            + 320.0 * std::sin(y * PI / 30.0)) * 2.0 / 3.0;

    return ret;
}

static double transformLng(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x
                 + 0.1 * x * y + 0.1 * std::sqrt(std::abs(x));

    ret += (20.0 * std::sin(6.0 * x * PI)
            + 20.0 * std::sin(2.0 * x * PI)) * 2.0 / 3.0;

    ret += (20.0 * std::sin(x * PI)
            + 40.0 * std::sin(x / 3.0 * PI)) * 2.0 / 3.0;

    ret += (150.0 * std::sin(x / 12.0 * PI)
            + 300.0 * std::sin(x / 30.0 * PI)) * 2.0 / 3.0;

    return ret;
}

Coord wgs84ToGcj02(double lat, double lng)
{
    if (outOfChina(lat, lng)) {
        return {lat, lng};
    }

    double dLat = transformLat(lng - 105.0, lat - 35.0);
    double dLng = transformLng(lng - 105.0, lat - 35.0);

    double radLat = lat / 180.0 * PI;
    double magic = std::sin(radLat);
    magic = 1.0 - EE * magic * magic;

    double sqrtMagic = std::sqrt(magic);

    dLat = (dLat * 180.0) /
           ((A * (1.0 - EE)) / (magic * sqrtMagic) * PI);

    dLng = (dLng * 180.0) /
           (A / sqrtMagic * std::cos(radLat) * PI);

    return {
        lat + dLat,
        lng + dLng
    };
}