#pragma once

#include <unordered_map>
#include <string>

namespace mina
{

class DisjointSetUnion
{
private:
    std::unordered_map<std::string, std::string> parent;

public:
    void make_set(const std::string& v);
    std::string find(const std::string& v);
    void unite(const std::string& u, const std::string& v);
};

}  // namespace mina
