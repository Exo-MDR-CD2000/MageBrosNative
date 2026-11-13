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
    {'C', "00000", "a-b-d"},
    {'C', "00000", "h$c$h"},
    {'-', "00100", "x-j-m"},
    {'C', "01110", "e-f-k"},
    {'-', "01210", "g-o-p"},
    {'W', "01110", "z-u-+"},
    {'-', "00100", "a-b-d"},
    {'C', "00100", "-P-T-"},
    {'-', "01110", "U-Q-O"},
    {'C', "01210", "//-//"},
    {'-', "01110", "E-F-G"},
    {'C', "00100", "*-H-*"},
    {'W', "00100", "$-i-$"},
    {'-', "00000", "i-i-i"},
    {'C', "00000", "h-H-e"},
    {'-', "00100", "-$-a-"},
    {'C', "01110", "f-k-g"},
    {'-', ".010.", "-----"},
    {'C', "..0..", "--$--"},
    {'-', "00000", "/-a-/"},
    {'C', "00000", "h-c-e"},
    {'-', "00100", "f-k-g"},
    {'C', "00000", "x-j-m"},
    {'-', "00000", "o-p-z"},
    {'C', "00000", "-u-i-"},
    {'-', "00000", "P-T-U"},
    {'C', "00000", "N-O-Q"},
    {'-', "00000", "A-B-C"},
    {'C', "00000", "D-E-F"},
    {'-', "00000", "G-+-$"},
    {'C', "00000", "a-b-d"},
    {'-', "00000", "h-c-e"},
    {'C', "00000", "f-k-g"},
    {'-', "00000", "x-j-m"},
    {'C', "00000", "o-p-z"},
    {'-', "00000", "-u-i-"},
    {'C', "00000", "-----"},
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
