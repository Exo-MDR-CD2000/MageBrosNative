#include "xfile.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>

namespace {

struct Token {
    enum Type { Ident, Number, String, Open, Close, End } type;
    std::string text;
    double value = 0;
};

class Lexer {
public:
    Lexer(const char* p, const char* e) : p_(p), e_(e) {}

    // Separators (';' and ',') carry no meaning for the data we read and are skipped.
    Token Next() {
        for (;;) {
            while (p_ < e_ && (std::isspace((unsigned char)*p_) || *p_ == ';' || *p_ == ',')) p_++;
            if (p_ >= e_) return {Token::End, {}};
            if (*p_ == '#' || (*p_ == '/' && p_ + 1 < e_ && p_[1] == '/')) {
                while (p_ < e_ && *p_ != '\n') p_++;
                continue;
            }
            break;
        }
        char c = *p_;
        if (c == '{') { p_++; return {Token::Open, "{"}; }
        if (c == '}') { p_++; return {Token::Close, "}"}; }
        if (c == '"') {
            const char* s = ++p_;
            while (p_ < e_ && *p_ != '"') p_++;
            Token t{Token::String, std::string(s, p_)};
            if (p_ < e_) p_++;
            return t;
        }
        if (c == '-' || c == '+' || c == '.' || std::isdigit((unsigned char)c)) {
            const char* s = p_;
            p_++;
            while (p_ < e_ && (std::isdigit((unsigned char)*p_) || *p_ == '.' || *p_ == 'e' ||
                               *p_ == 'E' || ((*p_ == '-' || *p_ == '+') && (p_[-1] == 'e' || p_[-1] == 'E'))))
                p_++;
            Token t{Token::Number, std::string(s, p_)};
            t.value = std::strtod(t.text.c_str(), nullptr);
            return t;
        }
        if (std::isalpha((unsigned char)c) || c == '_' || c == '<') {
            const char* s = p_;
            if (c == '<') {  // a GUID in a template declaration
                while (p_ < e_ && *p_ != '>') p_++;
                if (p_ < e_) p_++;
                return {Token::Ident, std::string(s, p_)};
            }
            while (p_ < e_ && (std::isalnum((unsigned char)*p_) || *p_ == '_' || *p_ == '-' || *p_ == '.')) p_++;
            return {Token::Ident, std::string(s, p_)};
        }
        p_++;  // anything else (stray punctuation) is skipped
        return Next();
    }

    Token Peek() {
        const char* save = p_;
        Token t = Next();
        p_ = save;
        return t;
    }

private:
    const char* p_;
    const char* e_;
};

class Parser {
public:
    Parser(const char* p, const char* e) : lx_(p, e) {}
    std::string error;

    bool ParseFile(XModel* model) {
        model_ = model;
        for (;;) {
            Token t = lx_.Next();
            if (t.type == Token::End) return true;
            if (t.type != Token::Ident) return Fail("expected a template name");
            if (!ParseObject(t.text, nullptr)) return false;
        }
    }

private:
    Lexer lx_;
    XModel* model_ = nullptr;
    std::map<std::string, XMaterial> namedMaterials_;

    bool Fail(const std::string& m) {
        if (error.empty()) error = m;
        return false;
    }

    // Reads "Type [Name] {" after the type name has been consumed. Returns the optional name.
    bool OpenBlock(std::string* name) {
        Token t = lx_.Next();
        if (t.type == Token::Ident) {
            *name = t.text;
            t = lx_.Next();
        }
        if (t.type != Token::Open) return Fail("expected '{'");
        return true;
    }

    bool ReadNumber(double* v) {
        Token t = lx_.Next();
        if (t.type != Token::Number) return Fail("expected a number");
        *v = t.value;
        return true;
    }
    bool ReadCount(uint32_t* n, uint32_t limit = 10000000) {
        double v;
        if (!ReadNumber(&v)) return false;
        if (v < 0 || v > limit) return Fail("bad count");
        *n = (uint32_t)v;
        return true;
    }
    bool ReadFloats(std::vector<float>* out, size_t n) {
        out->reserve(out->size() + n);
        for (size_t i = 0; i < n; i++) {
            double v;
            if (!ReadNumber(&v)) return false;
            out->push_back((float)v);
        }
        return true;
    }
    bool ReadFaces(std::vector<std::vector<uint32_t>>* faces) {
        uint32_t n;
        if (!ReadCount(&n)) return false;
        faces->resize(n);
        for (auto& f : *faces) {
            uint32_t k;
            if (!ReadCount(&k, 1024)) return false;
            f.resize(k);
            for (auto& idx : f)
                if (!ReadCount(&idx)) return false;
        }
        return true;
    }

    // Skips the rest of a block whose '{' has been consumed.
    bool SkipBlock() {
        int depth = 1;
        while (depth > 0) {
            Token t = lx_.Next();
            if (t.type == Token::End) return Fail("unexpected end of file");
            if (t.type == Token::Open) depth++;
            if (t.type == Token::Close) depth--;
        }
        return true;
    }

    // Nested data objects inside a block, up to its closing '}'.
    template <typename F>
    bool ParseChildren(F onChild) {
        for (;;) {
            Token t = lx_.Next();
            if (t.type == Token::Close) return true;
            if (t.type == Token::End) return Fail("unexpected end of file");
            if (t.type == Token::Open) {  // a reference: { Name }
                Token r = lx_.Next();
                if (r.type == Token::Ident || r.type == Token::String) onChild("", r.text);
                if (!SkipBlock()) return false;
                continue;
            }
            if (t.type != Token::Ident) continue;
            if (!onChild(t.text, std::string())) return false;
        }
    }

    bool ParseMaterial(XMaterial* m) {
        std::string name;
        if (!OpenBlock(&name)) return false;
        m->name = name;
        std::vector<float> f;
        if (!ReadFloats(&f, 4 + 1 + 3 + 3)) return false;
        std::memcpy(m->diffuse, &f[0], 4 * sizeof(float));
        m->power = f[4];
        std::memcpy(m->specular, &f[5], 3 * sizeof(float));
        std::memcpy(m->emissive, &f[8], 3 * sizeof(float));
        return ParseChildren([&](const std::string& type, const std::string&) {
            if (type == "TextureFilename" || type == "TextureFileName") {
                std::string n;
                if (!OpenBlock(&n)) return false;
                Token s = lx_.Next();
                if (s.type == Token::String) m->texture = s.text;
                return s.type == Token::Close ? true : SkipBlock();
            }
            std::string n;
            return OpenBlock(&n) && SkipBlock();
        });
    }

    bool ParseMaterialList(XMesh* mesh) {
        std::string name;
        if (!OpenBlock(&name)) return false;
        uint32_t nMat, nFace;
        if (!ReadCount(&nMat) || !ReadCount(&nFace)) return false;
        mesh->faceMaterials.resize(nFace);
        for (auto& i : mesh->faceMaterials)
            if (!ReadCount(&i)) return false;
        return ParseChildren([&](const std::string& type, const std::string& ref) {
            if (type.empty()) {  // reference to a top-level material
                auto it = namedMaterials_.find(ref);
                mesh->materials.push_back(it != namedMaterials_.end() ? it->second : XMaterial{});
                return true;
            }
            if (type == "Material") {
                XMaterial m;
                if (!ParseMaterial(&m)) return false;
                mesh->materials.push_back(m);
                return true;
            }
            std::string n;
            return OpenBlock(&n) && SkipBlock();
        });
    }

    bool ParseMesh() {
        XMesh mesh;
        if (!OpenBlock(&mesh.name)) return false;
        uint32_t nv;
        if (!ReadCount(&nv) || !ReadFloats(&mesh.positions, (size_t)nv * 3) || !ReadFaces(&mesh.faces))
            return false;
        for (const auto& f : mesh.faces)
            for (uint32_t i : f)
                if (i >= nv) return Fail("mesh " + mesh.name + ": vertex index out of range");
        bool ok = ParseChildren([&](const std::string& type, const std::string&) {
            std::string n;
            if (type == "MeshNormals") {
                uint32_t nn;
                if (!OpenBlock(&n) || !ReadCount(&nn) || !ReadFloats(&mesh.normals, (size_t)nn * 3) ||
                    !ReadFaces(&mesh.normalFaces))
                    return false;
                return lx_.Next().type == Token::Close ? true : SkipBlock();
            }
            if (type == "MeshTextureCoords") {
                uint32_t nt;
                if (!OpenBlock(&n) || !ReadCount(&nt) || !ReadFloats(&mesh.texcoords, (size_t)nt * 2))
                    return false;
                return lx_.Next().type == Token::Close ? true : SkipBlock();
            }
            if (type == "MeshMaterialList") return ParseMaterialList(&mesh);
            return OpenBlock(&n) && SkipBlock();
        });
        if (!ok) return false;
        model_->meshes.push_back(std::move(mesh));
        return true;
    }

    bool ParseObject(const std::string& type, void*) {
        if (type == "Mesh") return ParseMesh();
        if (type == "Material") {  // top-level, referenced by name later
            XMaterial m;
            if (!ParseMaterial(&m)) return false;
            namedMaterials_[m.name] = m;
            return true;
        }
        if (type == "Frame") {  // walk into frames for their meshes (transforms aren't applied)
            std::string n;
            if (!OpenBlock(&n)) return false;
            return ParseChildren([&](const std::string& t, const std::string&) {
                if (t.empty()) return true;
                return ParseObject(t, nullptr);
            });
        }
        if (type == "template") {  // template declarations: skip
            std::string n;
            return OpenBlock(&n) && SkipBlock();
        }
        std::string n;  // Header, FrameTransformMatrix, AnimationSet, ...
        return OpenBlock(&n) && SkipBlock();
    }
};

}  // namespace

size_t XModel::TriangleCount() const {
    size_t n = 0;
    for (const auto& m : meshes)
        for (const auto& f : m.faces)
            if (f.size() >= 3) n += f.size() - 2;
    return n;
}

bool ParseXFile(const uint8_t* data, size_t size, XModel* out, std::string* error) {
    if (size < 16 || std::memcmp(data, "xof ", 4) != 0) {
        if (error) *error = "not a .x file";
        return false;
    }
    if (std::memcmp(data + 8, "txt ", 4) != 0) {
        if (error) *error = "only text .x files are supported";
        return false;
    }
    const char* p = (const char*)data + 16;
    const char* e = (const char*)data + size;
    for (const char* q = p; q < e; q++)  // stop at an embedded terminator
        if (*q == '\0') { e = q; break; }
    *out = XModel{};
    Parser parser(p, e);
    if (!parser.ParseFile(out)) {
        if (error) *error = parser.error;
        return false;
    }
    for (const auto& m : out->meshes)
        if (!m.faceMaterials.empty() && m.faceMaterials.size() != m.faces.size() && m.faceMaterials.size() != 1) {
            if (error) *error = "mesh " + m.name + ": material list does not match faces";
            return false;
        }
    return true;
}
