#include "level.h"

std::vector<std::string> SplitLevelLines(const uint8_t* data, size_t size) {
    std::vector<std::string> lines;
    std::string cur;
    bool any = false;
    for (size_t i = 0; i < size; i++) {
        char c = (char)data[i];
        if (c == '\0') break;  // the resource is read as a C string
        if (c == '\r') continue;
        if (c == '\n') {
            lines.push_back(cur);
            cur.clear();
            any = false;
            continue;
        }
        cur += c;
        any = true;
    }
    if (any) lines.push_back(cur);
    return lines;
}

bool ParseLevel(const uint8_t* data, size_t size, LevelGrid* out, std::string* error) {
    std::vector<std::string> lines = SplitLevelLines(data, size);

    // Rows run up to the first empty line or the first line starting with '_' (the terminator).
    std::vector<const std::string*> rows;
    for (const auto& l : lines) {
        if (l.empty() || l[0] == '_') break;
        rows.push_back(&l);
    }
    if (rows.empty()) {
        if (error) *error = "level has no rows";
        return false;
    }

    const int w = LevelGrid::kWidth;
    out->depth = (int)rows.size();
    out->rowFlags.assign(out->depth, 0);
    out->cells.assign(out->depth * w, 0);
    out->objects.assign(out->depth * w, 0);

    // Position past the end of a short line reads as 0, like reading past a C string's end.
    auto at = [](const std::string& s, int i) -> char { return i < (int)s.size() ? s[i] : 0; };
    for (int row = 0; row < out->depth; row++) {
        const std::string& s = *rows[out->depth - row - 1];  // last line is row 0
        out->rowFlags[row] = at(s, 0);
        for (int x = 0; x < w; x++) {
            out->cells[row * w + x] = at(s, 2 + x);
            out->objects[row * w + x] = at(s, 8 + x);
        }
    }
    return true;
}
