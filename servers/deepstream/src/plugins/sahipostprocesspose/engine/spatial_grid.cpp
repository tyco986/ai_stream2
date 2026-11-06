#include "plugins/sahipostprocesspose/engine/spatial_grid.hpp"

#include <algorithm>
#include <cmath>

SpatialGrid::SpatialGrid()
    : cellSize(1.0f),
      columns(0),
      rows(0),
      generation(0) {
}

void SpatialGrid::build(const std::vector<GridRect>& boxes, float frameWidth, float frameHeight) {
    const unsigned int count = boxes.size();
    cellSize = 1.0f;
    columns = 0;
    rows = 0;
    cells.clear();
    stamp.clear();
    generation = 0;
    if (count > 0) {
        float maxDim = 0.0f;
        for (const GridRect& box : boxes) {
            const float width = box.right - box.left;
            const float height = box.bottom - box.top;
            maxDim = std::max(maxDim, std::max(width, height));
        }
        cellSize = std::max(maxDim, 1.0f);
        columns = static_cast<unsigned int>(std::ceil(frameWidth / cellSize)) + 1;
        rows = static_cast<unsigned int>(std::ceil(frameHeight / cellSize)) + 1;
        cells.resize(columns * rows);
        for (unsigned int index = 0; index < count; index++) {
            unsigned int column0 = 0;
            unsigned int row0 = 0;
            unsigned int column1 = 0;
            unsigned int row1 = 0;
            cellRange(boxes[index], column0, row0, column1, row1);
            for (unsigned int row = row0; row <= row1; row++) {
                for (unsigned int column = column0; column <= column1; column++) {
                    cells[row * columns + column].push_back(index);
                }
            }
        }
        stamp.assign(count, 0);
    }
}

void SpatialGrid::query(const GridRect& box, std::vector<unsigned int>& result) const {
    result.clear();
    if (columns > 0 && rows > 0) {
        unsigned int column0 = 0;
        unsigned int row0 = 0;
        unsigned int column1 = 0;
        unsigned int row1 = 0;
        generation++;
        cellRange(box, column0, row0, column1, row1);
        for (unsigned int row = row0; row <= row1; row++) {
            for (unsigned int column = column0; column <= column1; column++) {
                for (unsigned int index : cells[row * columns + column]) {
                    if (stamp[index] != generation) {
                        stamp[index] = generation;
                        result.push_back(index);
                    }
                }
            }
        }
    }
}

void SpatialGrid::cellRange(const GridRect& box, unsigned int& column0, unsigned int& row0, unsigned int& column1, unsigned int& row1) const {
    column0 = static_cast<unsigned int>(std::max(0.0f, box.left / cellSize));
    row0 = static_cast<unsigned int>(std::max(0.0f, box.top / cellSize));
    column1 = std::min(static_cast<unsigned int>(box.right / cellSize), columns - 1);
    row1 = std::min(static_cast<unsigned int>(box.bottom / cellSize), rows - 1);
}
