#ifndef DSU_H
#define DSU_H

#include <vector>

// ============================================================
// DSU (Disjoint Set Union / Union-Find).
// Kruskal dùng để trả lời nhanh: "hai node đã cùng một thành phần chưa?"
// Nếu rồi (cùng root) thì nối thêm một cạnh sẽ tạo CHU TRÌNH => bỏ qua.
//
// Hai kỹ thuật làm nó gần như O(1):
//   - Path compression : khi find(x), trỏ mọi node trên đường đi thẳng lên root.
//   - Union by rank    : gắn cây thấp vào cây cao, giữ cây không bị cao.
//
// Phần tử là số nguyên 0..n-1. Với đồ thị có id node lớn nhất là M thì tạo DSU(M + 1).
// ============================================================
class DSU {
public:
    explicit DSU(int n = 0) { reset(n); }

    // Khởi tạo lại n phần tử rời rạc: mỗi node là root của chính nó
    void reset(int n);

    // Root của tập chứa x (có path compression nên không const)
    int find(int x);

    // Gộp tập chứa a và b.
    // Trả true  nếu a, b trước đó KHÁC tập (đã gộp),
    //     false nếu đã cùng tập (không làm gì - gộp nữa sẽ tạo chu trình).
    bool unite(int a, int b);

    // Hai phần tử có cùng tập không
    bool connected(int a, int b) { return find(a) == find(b); }

    // Số tập (thành phần liên thông) hiện tại
    int components() const { return components_; }

    int size() const { return static_cast<int>(parent_.size()); }

private:
    std::vector<int> parent_;
    std::vector<int> rank_;
    int              components_ = 0;
};

#endif // DSU_H
