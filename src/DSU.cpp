#include "DSU.h"
#include <utility>

void DSU::reset(int n) {
    parent_.assign(n, 0);
    rank_.assign(n, 0);
    for (int i = 0; i < n; ++i) parent_[i] = i;   // mỗi phần tử là root của chính nó
    components_ = n;
}

int DSU::find(int x) {
    // Bước 1: leo lên tìm root
    int root = x;
    while (parent_[root] != root) root = parent_[root];
    // Bước 2 (path compression): trỏ mọi node trên đường đi thẳng vào root
    while (parent_[x] != root) {
        int next = parent_[x];
        parent_[x] = root;
        x = next;
    }
    return root;
}

bool DSU::unite(int a, int b) {
    int ra = find(a);
    int rb = find(b);
    if (ra == rb) return false;                  // đã cùng tập: không gộp
    if (rank_[ra] < rank_[rb]) std::swap(ra, rb); // ra là cây cao hơn (hoặc bằng)
    parent_[rb] = ra;                            // gắn cây thấp vào cây cao
    if (rank_[ra] == rank_[rb]) ++rank_[ra];     // chỉ khi bằng nhau chiều cao mới tăng
    --components_;
    return true;
}
