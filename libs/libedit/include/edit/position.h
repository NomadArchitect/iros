#pragma once

namespace Edit {
struct Position {
    int row;
    int col;

    bool operator==(const Position& other) const = default;
    bool operator!=(const Position& other) const = default;
    bool operator<=(const Position& other) const { return *this == other || *this < other; }
    bool operator>=(const Position& other) const { return *this == other || *this > other; }

    bool operator<(const Position& other) const {
        if (this->row < other.row) {
            return true;
        }

        if (this->row == other.row) {
            return this->col < other.col;
        }
        return false;
    }

    bool operator>(const Position& other) const {
        if (this->row > other.row) {
            return true;
        }

        if (this->row == other.row) {
            return this->col > other.col;
        }
        return false;
    }
};
}
