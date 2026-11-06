#pragma once

#include <vector>

struct GridRect {
    float left;
    float top;
    float right;
    float bottom;
};

class SpatialGrid {
public:
    SpatialGrid();

    void build(const std::vector<GridRect>& boxes, float frameWidth, float frameHeight);
    void query(const GridRect& box, std::vector<unsigned int>& result) const;

private:
    void cellRange(const GridRect& box, unsigned int& column0, unsigned int& row0, unsigned int& column1, unsigned int& row1) const;

    float cellSize;
    unsigned int columns;
    unsigned int rows;
    std::vector<std::vector<unsigned int>> cells;
    mutable std::vector<unsigned int> stamp;
    mutable unsigned int generation;
};
