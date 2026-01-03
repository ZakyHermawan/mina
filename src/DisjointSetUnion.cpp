#include "DisjointSetUnion.hpp"

// Adds a new variable to the DSU, initially in its own set.
void DisjointSetUnion::make_set(const std::string& v)
{
    if (parent.find(v) == parent.end())
    {
        parent[v] = v;
    }
}

// Finds the representative (root) of the set containing variable v.
std::string DisjointSetUnion::find(const std::string& v)
{
    // If v is not in the map, add it.
    make_set(v);

    // Find the root with path compression for efficiency.
    if (parent[v] == v)
    {
        return v;
    }
    return parent[v] = find(parent[v]);
}

// Merges the sets containing variables u and v.
void DisjointSetUnion::unite(const std::string& u, const std::string& v)
{
    std::string root_u = find(u);
    std::string root_v = find(v);
    if (root_u != root_v)
    {
        // Point the root of v's set to the root of u's set.
        parent[root_v] = root_u;
    }
}
