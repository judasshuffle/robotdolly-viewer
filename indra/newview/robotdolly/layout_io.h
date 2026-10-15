#pragma once
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace robotdolly {
// Store only the reachable tree, using stable editor keys instead of enum
// ordinals or array indices. Both codecs are shared with standalone tests.
template<class Nodes, class EditorKey>
bool encodeLayout(const Nodes& nodes, EditorKey key, std::string& text) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "ROBOTDOLLY_LAYOUT 1\n" << std::setprecision(std::numeric_limits<float>::max_digits10);
    std::vector<bool> seen(nodes.size());
    std::function<bool(int, int)> visit = [&](int id, int parent) {
        if (id < 0 || id >= (int)nodes.size() || seen[id]) return false;
        seen[id] = true;
        const auto& n = nodes[id];
        if (!n.used || n.parent != parent) return false;
        if (!n.split) {
            const std::string name = key(n.editor);
            if (name.empty()) return false;
            out << "leaf " << std::quoted(name) << '\n';
            return true;
        }
        if (!std::isfinite(n.fraction) || n.fraction <= 0.f || n.fraction >= 1.f) return false;
        out << "split " << (n.vertical ? "vertical" : "horizontal") << ' ' << n.fraction << '\n';
        return visit(n.first, id) && visit(n.second, id);
    };
    if (!visit(0, -1)) return false;
    for (size_t i = 0; i < nodes.size(); ++i)
        if (nodes[i].used != seen[i]) return false;
    text = out.str();
    return true;
}

template<class Nodes, class EditorId>
bool decodeLayout(const std::string& text, EditorId editorId, Nodes& destination) {
    if (text.size() > 65536) return false;
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    std::string magic;
    int version = 0;
    if (!(in >> magic >> version) || magic != "ROBOTDOLLY_LAYOUT" || version != 1) return false;
    Nodes result{};
    int next = 0;
    std::function<int(int)> read = [&](int parent) -> int {
        if (next >= (int)result.size()) return -1;
        const int id = next++;
        auto& n = result[id];
        n.used = true;
        n.parent = parent;
        std::string kind;
        if (!(in >> kind)) return -1;
        if (kind == "leaf") {
            std::string name;
            if (!(in >> std::quoted(name))) return -1;
            n.editor = editorId(name);
            return n.editor < 0 ? -1 : id;
        }
        if (kind != "split") return -1;
        std::string axis;
        if (!(in >> axis >> n.fraction) || (axis != "vertical" && axis != "horizontal") ||
            !std::isfinite(n.fraction) || n.fraction <= 0.f || n.fraction >= 1.f) return -1;
        n.split = true;
        n.vertical = axis == "vertical";
        n.first = read(id);
        if (n.first < 0) return -1;
        n.second = read(id);
        return n.second < 0 ? -1 : id;
    };
    if (read(-1) != 0) return false;
    in >> std::ws;
    if (!in.eof()) return false;
    destination = result; // Malformed input must never partially replace a layout.
    return true;
}
}
