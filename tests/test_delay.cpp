#include <iostream>
#include <cassert>
#include <cmath>
#include "DelayModel.h"
#include "Config.h"

bool isClose(double a, double b) {
    return std::abs(a - b) < 1e-9;
}

int main() {
    std::cout << "========================================\n";
    std::cout << " TEST MODULE DELAY MODEL - B3.1 to B3.5 \n";
    std::cout << "========================================\n\n";

    // 1. Tạo Packet giả (1500 Byte)
    Packet pkt;
    pkt.sizeBytes = 1500;

    // 2. Tạo Edge giả (100 Mbps, Dài 100 km cáp quang, Load 0.5, Base Queue Delay = 1ms)
    Edge edge;
    edge.isUp = true;
    edge.mediaType = MediaType::FIBER;
    edge.bandwidthMbps = 100.0;
    edge.length = 100000.0; // 100,000 m = 100 km
    edge.queueDelay = 0.001; // 1 ms
    edge.currentLoad = 0.5; // Không bị nghẽn (penalty = 1.0)

    // 3. Tạo Node đích giả (Processing Time = 2ms)
    Node nextNode;
    nextNode.processingTime = 0.002;

    // 4. Test độc lập từng hàm
    double dTrans = DelayModel::calcTransmissionDelay(pkt.sizeBytes, edge.bandwidthMbps);
    // (1500 * 8) / (100 * 1e6) = 12000 / 100,000,000 = 0.00012 s
    std::cout << "Transmission Delay: " << dTrans << " s\n";
    assert(isClose(dTrans, 0.00012));

    double dProp = DelayModel::calcPropagationDelay(edge.length, Config::SPEED_OF_LIGHT_FIBER);
    // 100000 / 2e8 = 0.0005 s
    std::cout << "Propagation Delay : " << dProp << " s\n";
    assert(isClose(dProp, 0.0005));

    double dProc = DelayModel::calcProcessingDelay(nextNode);
    std::cout << "Processing Delay  : " << dProc << " s\n";
    assert(isClose(dProc, 0.002));

    double dQueue = DelayModel::calcQueueDelay(edge.queueDelay, edge.currentLoad);
    std::cout << "Queue Delay       : " << dQueue << " s\n";
    assert(isClose(dQueue, 0.001));

    // 5. Test tổng Base Delay
    double total = DelayModel::calcBaseDelay(pkt, edge, nextNode);
    double expectedTotal = 0.00012 + 0.0005 + 0.002 + 0.001;
    std::cout << "Total Base Delay  : " << total << " s\n";
    assert(isClose(total, expectedTotal));

    // 6. Test Fragmentation (Task B3.6, B3.7)
    // Gói tin 3000 Byte, MTU 1500 -> Payload = 1480 -> 3000 / 1480 = 2 dư 40 -> 3 mảnh
    Packet largePkt;
    largePkt.sizeBytes = 3000;
    largePkt.canFragment = true;
    largePkt.protocol = Protocol::UDP;
    largePkt.newConnection = false;
    
    int fragments = DelayModel::calcFragmentCount(largePkt.sizeBytes, 1500, largePkt.canFragment);
    std::cout << "\nFragments Count   : " << fragments << "\n";
    assert(fragments == 3);
    
    largePkt.canFragment = false; // Test rớt gói
    assert(DelayModel::calcFragmentCount(largePkt.sizeBytes, 1500, largePkt.canFragment) == 0);

    // 7. Test TCP / UDP Protocol Overhead (Task B3.8)
    Packet tcpPkt;
    tcpPkt.sizeBytes = 1000;
    tcpPkt.protocol = Protocol::TCP;
    tcpPkt.newConnection = true;
    tcpPkt.canFragment = true;
    
    double oneWay = 0.05; // 50 ms
    double overhead = DelayModel::calcProtocolOverhead(tcpPkt, oneWay);
    std::cout << "TCP Overhead (1 RTT): " << overhead << " s\n";
    assert(isClose(overhead, 0.1)); // 2 * 0.05 = 0.1
    
    // 8. Test CalculateTotalDelay
    tcpPkt.sizeBytes = 3000; // 3 mảnh -> dTrans * 3
    // Base delays: dTrans = 0.00012 (nhưng ở đây size 3000 -> dTrans_1_packet = 3000*8/100e6 = 0.00024)
    // => Fragments = 3 => Total Trans = 0.00024 * 3 = 0.00072
    // dProp = 0.0005
    // dProc = 0.002
    // dQueue = 0.001
    // One Way Delay = 0.00072 + 0.0005 + 0.002 + 0.001 = 0.00422
    // TCP New Conn Overhead = 2 * 0.00422 = 0.00844
    // Total = 0.00422 + 0.00844 = 0.01266 s
    
    edge.mtu = 1500;
    double grandTotal = DelayModel::calculateTotalDelay(tcpPkt, edge, nextNode);
    std::cout << "Grand Total Delay   : " << grandTotal << " s\n";
    assert(isClose(grandTotal, 0.01266));

    std::cout << "\n=> ALL MATH TESTS PASSED!\n";
    return 0;
}
