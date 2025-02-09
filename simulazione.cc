#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/gnuplot.h"

using namespace ns3;

// Oggetti Gnuplot2dDataset (non utilizzati nel plot)
Gnuplot2dDataset datasetTx("Pacchetti TX");
Gnuplot2dDataset datasetAck("ACK RX");
Gnuplot2dDataset datasetSsthresh("SSTHRESH");
Gnuplot2dDataset datasetCwnd("CWND");
Gnuplot2dDataset datasetRtt("RTT");
Gnuplot2dDataset datasetRto("RTO");

// Creazione dei dataset per tipo di dato
struct Data {
    double time;
    double value;
};

std::vector<Data> txData;
std::vector<Data> ackData;
std::vector<Data> ssthreshData;
std::vector<Data> cwndData;
std::vector<Data> rttData;
std::vector<Data> rtoData;


// Tracciamento dei pacchetti trasmessi
void PacketTrace(Ptr<const Packet> pkt, const TcpHeader &header, Ptr<const TcpSocketBase> tcpSocket) {

    double time = Simulator::Now().GetSeconds();
    uint32_t seq = header.GetSequenceNumber().GetValue();

    datasetTx.Add(time, seq);
    txData.push_back({time, static_cast<double>(seq)});

    std::cout << "[TX] Time: " << time << "s, Seq: " << seq << std::endl;
}

// Tracciamento degli ack ricevuti
void AckTrace(Ptr<const Packet> pkt, const TcpHeader &header, Ptr<const TcpSocketBase> tcpSocket) {

    double time = Simulator::Now().GetSeconds();
    uint32_t seq = header.GetAckNumber().GetValue();

    datasetAck.Add(time, seq);
    ackData.push_back({time, static_cast<double>(seq)});

    std::cout << "[Ack] Time: " << time << "s, Seq: " << seq << std::endl;
}

// Tracciamento della congestion window
void CongestionWindowTrace(uint32_t oldCwnd, uint32_t newCwnd) {
    std::cout << "[DEBUG] CongestionWindowTrace called. Time: " << Simulator::Now().GetSeconds() << ", OldCwnd: " << oldCwnd << ", NewCwnd: " << newCwnd/1024 << std::endl;
    double time = Simulator::Now().GetSeconds();

    datasetCwnd.Add(time, (uint32_t)oldCwnd);
    datasetCwnd.Add(time, (uint32_t)newCwnd);
    cwndData.push_back({time, static_cast<double>(oldCwnd)});
    cwndData.push_back({time, static_cast<double>(newCwnd)});

    std::cout << "[CWND] Time: " << time << "s, Cwnd: " << newCwnd << std::endl;
}

// Tracciamento della soglia di slow start 
void SsthreshTrace(uint32_t oldSsthresh, uint32_t newSsthresh) {
    std::cout << "[DEBUG] SsthreshTrace called. Time: " << Simulator::Now().GetSeconds() << ", OldSsthresh: " << oldSsthresh << ", NewSsthresh: " << newSsthresh/1024 << std::endl;
    double time = Simulator::Now().GetSeconds();

    datasetSsthresh.Add(time, (uint32_t)oldSsthresh);
    datasetSsthresh.Add(time, (uint32_t)newSsthresh);
    ssthreshData.push_back({time, static_cast<double>(oldSsthresh)});
    ssthreshData.push_back({time, static_cast<double>(newSsthresh)});

    std::cout << "[SSTHRESH] Time: " << time << "s, Ssthresh: " << newSsthresh << std::endl;
}

// Tracciamento del Round Trip Time (RTT)
void RttTrace(Time oldRtt, Time newRtt) {
    double time = Simulator::Now().GetSeconds();

    datasetRtt.Add(time, oldRtt.GetMilliSeconds());
    datasetRtt.Add(time, newRtt.GetMilliSeconds());
    rttData.push_back({time, static_cast<double>(oldRtt.GetMilliSeconds())});
    rttData.push_back({time, static_cast<double>(newRtt.GetMilliSeconds())});

    std::cout << "[RTT] Time: " << time << "s, RTT: " << newRtt.GetMilliSeconds() << "ms" << std::endl;
}

// Tracciamento del Retransmission Timeout
void RtoTrace(Time oldRto, Time newRto) {
    double time = Simulator::Now().GetSeconds();

    datasetRto.Add(time, oldRto.GetMilliSeconds());
    datasetRto.Add(time, newRto.GetMilliSeconds());
    rtoData.push_back({time, static_cast<double>(oldRto.GetMilliSeconds())});
    rtoData.push_back({time, static_cast<double>(newRto.GetMilliSeconds())});

    std::cout << "[RTO] Time: " << time << "s, RTO: " << newRto.GetMilliSeconds() << "ms" << std::endl;
}

// Connessione delle callback del tracciamento alla socket
void ConnectTrace(Ptr<OnOffApplication> OnOffApp)
{
    Ptr<Socket> socket = OnOffApp->GetSocket();
    socket->TraceConnectWithoutContext("Tx", MakeCallback(&PacketTrace));
    socket->TraceConnectWithoutContext("Rx", MakeCallback(&AckTrace));

    std::cout << "[DEBUG] Connecting CongestionWindow trace" << std::endl;
    socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CongestionWindowTrace));
    std::cout << "[DEBUG] Connecting SlowStartThreshold trace" << std::endl;
    socket->TraceConnectWithoutContext("SlowStartThreshold", MakeCallback(&SsthreshTrace));
    std::cout << "[DEBUG] Connecting RTT trace" << std::endl;
    socket->TraceConnectWithoutContext("RTT", MakeCallback(&RttTrace));
    std::cout << "[DEBUG] Connecting RTO trace" << std::endl;
    socket->TraceConnectWithoutContext("RTO", MakeCallback(&RtoTrace));
}


// Funzione per scrivere i dati in un file csv
void WriteDataPointsToCSV(const std::vector<Data>& data, const std::string& filename) {
    std::ofstream csvFile(filename);
    if (!csvFile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Scrive l'header
    csvFile << "Time,Value\n";

    // Scrive i dati
    for (const auto& point : data) {
        csvFile << point.time << "," << point.value << "\n";
    }

    csvFile.close();
    std::cout << "Data written to " << filename << std::endl;
}


int main(int argc, char *argv[]) {
    //LogComponentEnable("TcpVegas", LOG_LEVEL_INFO);
    //LogComponentEnable("TcpWestwood", LOG_LEVEL_INFO);
    //LogComponentEnable("TcpVeno", LOG_LEVEL_INFO);
    
    // Creazione dei nodi
    NodeContainer nodes;
    nodes.Create(2);

    // Creazione e configurazione del canale
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    // Creazione del modello di errore, installazione dei dei net device e assegnazione del modello di errore
    Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
    errorModel->SetAttribute("ErrorRate", DoubleValue(0.0001));

    NetDeviceContainer devices = p2p.Install(nodes);
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));

    // Installazione della pila protocollare
    InternetStackHelper stack;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpVegas"));
    stack.Install(nodes);

    // Assegnazione degli indirizzi IP
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    uint16_t port = 8080;
    Address serverAddress(InetSocketAddress(interfaces.GetAddress(1), port));
    
    // Creazione dell'app server TCP
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(20));

    // Creazione e setting dell'app client TCP
    OnOffHelper sender("ns3::TcpSocketFactory", serverAddress);
    sender.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    sender.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    //sender.SetAttribute("PacketSize", UintegerValue(1024)); // Set packet size to 1024 bytes
    //sender.SetAttribute("DataRate", StringValue("2Mbps"));
    ApplicationContainer senderApp = sender.Install(nodes.Get(0));
    senderApp.Start(Seconds(1.0));
    senderApp.Stop(Seconds(20));

    // Chiamata alla funzione ConnectTrace dopo l'avvio del client
    Ptr<OnOffApplication> OnOffApp = DynamicCast<OnOffApplication>(senderApp.Get(0));
    Simulator::Schedule(Seconds(1.0001), &ConnectTrace, OnOffApp);

    Simulator::Run();

    // Creazione del grafico con Gnuplot
    Gnuplot gnuplot("/Users/andrea/Desktop/tcp_plot.png");
    gnuplot.SetTitle("TCP Vegas Trace");
    gnuplot.SetTerminal("png");
    gnuplot.SetLegend("Tempo (s)", "Numero di Sequenza");

    gnuplot.SetTerminal("png size 1280,720"); // Dimensioni del grafico

    gnuplot.AppendExtra("set grid"); // Attiva la griglia
    gnuplot.AppendExtra("set datafile separator ','"); // Imposta il separatore delle colonne del dataset
    
    gnuplot.AppendExtra("set xrange [4.9:5.2]"); // Intervallo asse x
    gnuplot.AppendExtra("set xtics nomirror"); // Evita il mirroring dell'asse x
    gnuplot.AppendExtra("set yrange [160000:220000]"); // Intervallo dell'asse y1
    gnuplot.AppendExtra("set ytics nomirror"); // Evita il mirroring dell'asse y1
    gnuplot.AppendExtra("set y2range [0:20000]"); // Intervallo dell'asse y2
    gnuplot.AppendExtra("set y2tics"); // Attiva le etichette dell'asse y2
    gnuplot.AppendExtra("set y2label 'Bytes'"); // Nome delle etichette per l'asse y2

    /* Plotting degli oggetti Gnuplot2dDataset (se non si utilizzano assi multipli)
    
    datasetTx.SetStyle(Gnuplot2dDataset::POINTS);
    datasetTx.SetExtra("pointtype 7 pointsize 0.50 linecolor rgb 'blue'");

    datasetAck.SetStyle(Gnuplot2dDataset::POINTS);
    datasetAck.SetExtra("pointtype 9 pointsize 0.50 linecolor rgb 'green'");

    datasetCwnd.SetStyle(Gnuplot2dDataset::LINES); 
    datasetCwnd.SetExtra("linewidth 2 linecolor rgb 'black'");

    datasetSsthresh.SetStyle(Gnuplot2dDataset::LINES);
    datasetSsthresh.SetExtra("linewidth 2 linecolor rgb 'red'");

    gnuplot.AddDataset(datasetTx);
    gnuplot.AddDataset(datasetAck);
    gnuplot.AddDataset(datasetCwnd);
    gnuplot.AddDataset(datasetSsthresh);

    */

    // Creazione del file di output
    std::ofstream plotFile("/Users/andrea/Desktop/tcp_plot.plt");
    gnuplot.GenerateOutput(plotFile);

    // Scrive i diversi dataset in diversi file csv
    WriteDataPointsToCSV(txData, "/Users/andrea/Desktop/datasetTx.csv");
    WriteDataPointsToCSV(ackData, "/Users/andrea/Desktop/datasetAck.csv");
    WriteDataPointsToCSV(cwndData, "/Users/andrea/Desktop/datasetCwnd.csv");
    WriteDataPointsToCSV(ssthreshData, "/Users/andrea/Desktop/datasetSsthresh.csv");
    WriteDataPointsToCSV(rttData, "/Users/andrea/Desktop/datasetRtt.csv");
    WriteDataPointsToCSV(rtoData, "/Users/andrea/Desktop/datasetRto.csv");

    // Plotting dei file csv nel grafico
    plotFile << "plot ";
    plotFile << "'/Users/andrea/Desktop/datasetTx.csv' using 1:2 with points title 'Pacchetti TX', ";
    plotFile << "'/Users/andrea/Desktop/datasetAck.csv' using 1:2 with points title 'ACK RX', ";
    plotFile << "'/Users/andrea/Desktop/datasetCwnd.csv' using 1:2 axes x1y2 with lines linecolor 'blue' title 'CWND', ";
    plotFile << "'/Users/andrea/Desktop/datasetSsthresh.csv' using 1:2 axes x1y2 with lines linecolor 'red' title 'SSTHRESH';\n";
    //plotFile << "'/Users/andrea/Desktop/datasetRtt.csv' using 1:2 axes x1y2 with lines linecolor 'black' title 'RTT', ";
    //plotFile << "'/Users/andrea/Desktop/datasetRto.csv' using 1:2 axes x1y2 with lines linecolor 'grey' title 'RTO';\n";
    
    plotFile.close();

    Simulator::Destroy();
    return 0;
}

