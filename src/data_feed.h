#pragma once
#include <string>
#include <vector>

// One row of Apple (AAPL.O) market data
struct Tick {
    std::string date;
    double price;
};

// Load AAPL.O prices from tr_eikon_eod_data.csv.
// Rows with a missing price are silently skipped.
std::vector<Tick> loadCSV(const std::string& filename);
