// Airport and runway lookup: built-in OurAirports data first, then X-Plane apt.dat files.
#pragma once
#include <optional>
#include <string>
#include <vector>

namespace pfm {

struct RunwayEnd {
    std::string id;   // e.g. "06L"
    double len = 0;   // m
    int mag = 0;      // magnetic heading approximated from the runway number
    int tru = 0;      // true heading
    double slope = 0; // percent, positive = uphill in the takeoff direction
};

struct Airport {
    std::string icao, iata, name, city, src;
    int elevFt = 0;
    std::vector<RunwayEnd> ends;
};

// X-Plane's root folder (with trailing separator). Needed only for the apt.dat fallback.
void set_system_path(const std::string& path);

// code = 3-letter IATA or 4-letter ICAO, upper case. Results are cached.
const Airport* find_airport(const std::string& code);

int apt_files_searched();
int apt_files_opened();

// Exposed for tests.
std::optional<Airport> db_lookup(const std::string& code);
std::optional<Airport> read_apt_dat(const std::string& path, const std::string& icao);

}  // namespace pfm
