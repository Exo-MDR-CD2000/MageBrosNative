#include "benchmark_level.h"

namespace BenchmarkLevel {

namespace {

struct Row {
    char flag;
    const char* cells;
    const char* objects;
};

// Rows nearest the start first. The opening view covers roughly rows 0..14 at 4:3; these carry
// the dense mix. Rows 19..21 contain pits for cliff faces. The tail is a normal walk to the goal.
const Row kRows[kDepth] = {
    {'-', "00000", "--$--"},
    {'C', "00000", "-*a*-"},
    {'-', "00000", "a----"},
    {'-', "00000", "h-$--"},
    {'-', "00100", "x----"},
    {'-', "01110", "e----"},
    {'-', "01210", "g----"},
    {'W', "01110", "z----"},
    {'-', "00100", "a----"},
    {'-', "00100", "-----"},
    {'-', "01110", "U----"},
    {'-', "01210", "//---"},
    {'-', "01110", "E----"},
    {'-', "00100", "*-H-*"},
    {'W', "00100", "$-i-$"},
    {'-', "00000", "i----"},
    {'-', "00000", "h-H--"},
    {'-', "00100", "-$-a-"},
    {'-', "01110", "f----"},
    {'-', ".010.", "-----"},
    {'-', "..0..", "--$--"},
    {'-', "00000", "/-a--"},
    {'-', "00000", "h----"},
    {'-', "00100", "f---g"},
    {'-', "00000", "x---m"},
    {'-', "00000", "o---z"},
    {'-', "00000", "-u-i-"},
    {'-', "00000", "P----"},
    {'-', "00000", "N----"},
    {'-', "00000", "A-B--"},
    {'-', "00000", "D----"},
    {'-', "00000", "---+-$"},
    {'-', "00000", "a---d"},
    {'-', "00000", "h---e"},
    {'-', "00000", "f---g"},
    {'-', "00000", "x---m"},
    {'-', "00000", "o---z"},
    {'-', "00000", "-u-i-"},
    {'-', "00000", "-----"},
    {'-', "00000", "--#--"},
};

std::string BuildText() {
    std::string out;
    out.reserve(kDepth * 14);
    // The original's text is stored with the last row first: ParseLevel reads the last line as
    // row 0.
    for (int i = kDepth - 1; i >= 0; i--) {
        const Row& r = kRows[i];
        out += r.flag;
        out += '|';
        out += r.cells;
        out += '|';
        out += r.objects;
        out += '\n';
    }
    out += "_\n";
    return out;
}

}  // namespace

const std::string& Text() {
    static const std::string text = BuildText();
    return text;
}

bool Build(LevelGrid* out, std::string* error) {
    const std::string& text = Text();
    return ParseLevel(reinterpret_cast<const uint8_t*>(text.data()), text.size(), out, error);
}

uint32_t Hash() {
    const std::string& text = Text();
    uint32_t h = 2166136261u;
    for (unsigned char c : text) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

}  // namespace BenchmarkLevel
