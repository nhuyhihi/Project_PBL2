#ifndef DSU_H
#define DSU_H

#include <vector>

// Disjoint Set Union (Path Compression + Union by Rank) phục vụ kiểm tra chu trình
class DSU {
public:
    explicit DSU(int n = 0) { reset(n); }

    void reset(int n);
    int find(int x);
    bool unite(int a, int b);
    bool connected(int a, int b) { return find(a) == find(b); }
    int components() const { return components_; }
    int size() const { return static_cast<int>(parent_.size()); }

private:
    std::vector<int> parent_;
    std::vector<int> rank_;
    int              components_ = 0;
};

#endif // DSU_H
