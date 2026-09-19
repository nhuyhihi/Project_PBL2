#include <iostream>
#include <string>
#include "NetworkGraph.h"

using namespace std;

void printMenu() {
    cout << "\n============================================\n";
    cout << "  CHUONG TRINH MO PHONG VA TOI UU MANG\n";
    cout << "============================================\n";
    cout << "1. Nhap do thi mang tu file (vd: tests dijstra/test1.txt)\n";
    cout << "2. Tim duong di ngan nhat giua 2 Router (Dijkstra)\n";
    cout << "3. Tim phuong an ket noi mang toi uu (Kruskal)\n";
    cout << "0. Thoat chuong trinh\n";
    cout << "============================================\n";
    cout << "Nhap lua chon cua ban: ";
}

int main() {
    NetworkGraph* graph = nullptr;
    int choice;

    do {
        printMenu();
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Lua chon khong hop le, vui long nhap so!\n";
            continue;
        }

        switch (choice) {
            case 1: {
                string filename;
                cout << "Nhap duong dan file mang (vd: tests dijstra/test1.txt): ";
                cin >> ws; // xoa khoang trang thua
                getline(cin, filename);
                
                // Gia dinh mang thuc te luon la mang vo huong (cap 2 chieu)
                // Khoi tao do thi vo huong (isDirected = false)
                delete graph; // Xoa do thi cu neu co
                graph = new NetworkGraph(0, false);
                graph->loadFromFile(filename);
                cout << "=> Da doc file thanh cong (neu file ton tai).\n";
                break;
            }
            case 2: {
                if (graph == nullptr) {
                    cout << "Vui long nhap do thi tu file truoc (Chon 1).\n";
                    break;
                }
                int source, dest;
                cout << "Nhap Router nguon (Source): ";
                cin >> source;
                cout << "Nhap Router dich (Destination): ";
                cin >> dest;
                
                cout << "\n--- KET QUA DIJKSTRA ---\n";
                graph->dijkstra(source, dest);
                cout << "------------------------\n";
                break;
            }
            case 3: {
                if (graph == nullptr) {
                    cout << "Vui long nhap do thi tu file truoc (Chon 1).\n";
                    break;
                }
                cout << "\n--- KET QUA KRUSKAL (CAY KHUNG NHO NHAT) ---\n";
                vector<Edge> mst = graph->kruskal();
                
                if (!mst.empty()) {
                    cout << "Cac lien ket (cap) duoc chon:\n";
                    for (const Edge& e : mst) {
                        cout << "Router " << e.u << " <---> Router " << e.v << " : Chi phi " << e.w << endl;
                    }
                }
                cout << "------------------------------------------\n";
                break;
            }
            case 0:
                cout << "Dang thoat chuong trinh. Tam biet!\n";
                break;
            default:
                cout << "Lua chon khong hop le. Vui long thu lai.\n";
                break;
        }
    } while (choice != 0);

    delete graph;
    return 0;
}
