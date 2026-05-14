#include <string>
#include <cstddef>

std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string MakeJsonResponse(const std::string& id, const std::string& resultJson) {
    return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + resultJson + "}\n";
}

std::string MakeJsonError(const std::string& id, int code, const std::string& message) {
    return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"error\":{\"code\":" + std::to_string(code) + ",\"message\":\"" + JsonEscape(message) + "\"}}\n";
}

std::string ExtractMethod(const std::string& json) {
    auto pos = json.find("\"method\"");
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + 7);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

std::string ExtractId(const std::string& json) {
    auto pos = json.find("\"id\"");
    if (pos == std::string::npos) return "null";
    pos = json.find(':', pos + 3);
    if (pos == std::string::npos) return "null";
    pos = json.find_first_not_of(" \t", pos + 1);
    if (pos == std::string::npos) return "null";
    if (json[pos] == '"') {
        auto end = json.find('"', pos + 1);
        if (end == std::string::npos) return "null";
        return json.substr(pos, end - pos + 1);
    }
    auto end = json.find_first_of(",}\n", pos);
    if (end == std::string::npos) return json.substr(pos);
    std::string id = json.substr(pos, end - pos);
    while (!id.empty() && id.back() == ' ') id.pop_back();
    return id;
}

std::string ExtractArg(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos = json.find_first_not_of(" \t", pos + 1);
    if (pos == std::string::npos) return "";
    if (json[pos] == '"') {
        auto end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";
        std::string val = json.substr(pos + 1, end - pos - 1);
        std::string unescaped;
        for (size_t i = 0; i < val.size(); i++) {
            if (val[i] == '\\' && i + 1 < val.size()) {
                if (val[i+1] == 'n') unescaped += '\n';
                else if (val[i+1] == 't') unescaped += '\t';
                else if (val[i+1] == 'r') unescaped += '\r';
                else if (val[i+1] == '"') unescaped += '"';
                else if (val[i+1] == '\\') unescaped += '\\';
                else { unescaped += val[i]; unescaped += val[i+1]; }
                i++;
            } else {
                unescaped += val[i];
            }
        }
        return unescaped;
    }
    auto end = json.find_first_of(",}\n", pos);
    if (end == std::string::npos) return json.substr(pos);
    return json.substr(pos, end - pos);
}
