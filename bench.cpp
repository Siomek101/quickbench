#include <ncurses.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <numeric>
#include <immintrin.h>

using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;

/* ================= SYSTEM ================= */

string readFile(const string& p) {
    ifstream f(p);
    if (!f) return "";
    stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

string getCPUModel() {
    ifstream f("/proc/cpuinfo");
    string line;
    while (getline(f,line))
        if (line.find("model name")!=string::npos)
            return line.substr(line.find(":")+2);
    return "Unknown";
}

long getRAMUsedMB() {
    ifstream f("/proc/meminfo");
    string key;
    long value;
    string unit;
    long memTotal = 0, memFree = 0, buffers = 0, cached = 0;

    while(f >> key >> value >> unit) {
        if(key == "MemTotal:") memTotal = value;
        else if(key == "MemFree:") memFree = value;
        else if(key == "Buffers:") buffers = value;
        else if(key == "Cached:") cached = value;
        if(memTotal && memFree && buffers && cached) break;
    }

    long usedKB = memTotal - (memFree + buffers + cached);
    return usedKB / 1024; // MB
}

string getOS() {
    string os = readFile("/etc/os-release");
    size_t p = os.find("PRETTY_NAME=");
    if (p!=string::npos) {
        size_t s=os.find("\"",p);
        size_t e=os.find("\"",s+1);
        return os.substr(s+1,e-s-1);
    }
    return "Linux";
}

long getRAM() {
    ifstream f("/proc/meminfo");
    string k; long v; string u;
    while (f>>k>>v>>u)
        if (k=="MemTotal:")
            return v/1024;
    return 0;
}

double getTemp() {
    for (auto &e: fs::directory_iterator("/sys/class/hwmon")) {
        for(int i=1;i<10;i++){
            string p=e.path().string()+"/temp"+to_string(i)+"_input";
            ifstream f(p);
            if(f){ double t; f>>t; if(t>0) return t/1000.0;}
        }
    }
    return -1;
}

/* ================= CPU ================= */

double benchInt(int sec){
    volatile uint64_t x=1;
    auto end=high_resolution_clock::now()+seconds(sec);
    uint64_t ops=0;
    while(high_resolution_clock::now()<end){
        x = x*1664525 + 1013904223;
        ops++;
    }
    return ops/(double)sec/1e6;
}

double benchFloat(int sec){
    volatile double x=1;
    auto end=high_resolution_clock::now()+seconds(sec);
    uint64_t ops=0;
    while(high_resolution_clock::now()<end){
        x=sin(x)*cos(x)+sqrt(x);
        ops++;
    }
    return ops/(double)sec/1e6;
}

double benchAVX(int sec){
    if(!__builtin_cpu_supports("avx2")) return 0;
//    __m256 v=_mm256_set1_ps(1.1f);
//    auto end=high_resolution_clock::now()+seconds(sec);
//    uint64_t ops=0;
//    while(high_resolution_clock::now()<end){
//        v=_mm256_mul_ps(v,v);
//        ops++;
//    }
//    return ops/(double)sec/1e6;
    return -1;
}

double benchThreadsLevel(int sec,int threads){
    atomic<bool> stop(false);
    atomic<uint64_t> ops(0);

    auto worker=[&](){
        while(!stop) ops++;
    };

    vector<thread> t;
    for(int i=0;i<threads;i++)
        t.emplace_back(worker);

    this_thread::sleep_for(seconds(sec));
    stop=true;
    for(auto &th:t) th.join();

    return ops.load()/(double)sec/1e6;
}

/* ================= RAM ================= */

double benchMemory(size_t mb){
    size_t size=mb*1024*1024;
    vector<char> v(size);
    auto s=high_resolution_clock::now();
    for(size_t i=0;i<size;i++) v[i]=i;
    auto e=high_resolution_clock::now();
    return mb/duration<double>(e-s).count();
}

double benchLatency(size_t mb){
    size_t count=(mb*1024*1024)/sizeof(size_t);
    vector<size_t> v(count);
    for(size_t i=0;i<count-1;i++) v[i]=i+1;
    v[count-1]=0;
    size_t idx=0;
    auto s=high_resolution_clock::now();
    for(size_t i=0;i<count;i++) idx=v[idx];
    auto e=high_resolution_clock::now();
    return duration<double,nano>(e-s).count()/count;
}

/* ================= SCORE ================= */

double geoMean(vector<double> v){
    double sum=0; int c=0;
    for(double x:v){
        if(x>0){ sum+=log(x); c++; }
    }
    return exp(sum/c);
}

/* ================= STRESS ================= */

void stressMode(int sec){
    initscr(); noecho(); curs_set(0);
    auto end=high_resolution_clock::now()+seconds(sec);
    while(high_resolution_clock::now()<end){
        benchInt(1);
        clear();
        mvprintw(2,2,"STRESS MODE");
        mvprintw(4,2,"Temp: %.1f C",getTemp());
        refresh();
    }
    getch(); endwin();
}

/* ================= MAIN ================= */

int main(int argc,char* argv[]){

    bool md=false,json=false;

    if(argc>1){
        string arg=argv[1];
        if(arg=="--md") md=true;
        if(arg=="--json") json=true;
        if(arg=="--stress" && argc>2){
            stressMode(stoi(argv[2]));
            return 0;
        }
    }

    string cpu=getCPUModel();
    string os=getOS();
    long ramTotal=getRAM();
    int maxThreads=thread::hardware_concurrency();

    double intScore=benchInt(2);
    double floatScore=benchFloat(2);
    double avxScore=benchAVX(2);

    vector<int> levels={1,2,4,8,maxThreads};
    vector<double> mtScores;

    for(int t:levels){
        if(t<=maxThreads)
            mtScores.push_back(benchThreadsLevel(2,t));
        else
            mtScores.push_back(0);
    }

    double l1=benchMemory(1);
    double l2=benchMemory(4);
    double l3=benchMemory(16);
    double ram=benchMemory(512);
    double latency=benchLatency(128);

    double finalScore=geoMean({intScore,floatScore,avxScore,ram,mtScores.back()});

    /* -------- Markdown -------- */

    if(md){
        cout<<"# benchmark results\n\n";
        cout<<"## System\n";
        cout<<"- OS: "<<os<<"\n";
        cout<<"- CPU: "<<cpu<<"\n";
        cout<<"- RAM: "<<ramTotal<<" MB\n";
	cout<<"- RAM at idle: : " << getRAMUsedMB() << " MB\n\n";

        cout<<"## CPU\n";
        cout<<"- Integer: "<<intScore<<" Mops\n";
        cout<<"- Float: "<<floatScore<<" Mops\n";
        cout<<"- AVX2: "<<avxScore<<" Mops\n";

        for(size_t i=0;i<levels.size();i++)
            cout<<"- Threads "<<levels[i]<<": "<<mtScores[i]<<" Mops\n";

        cout<<"\n## Memory\n";
        cout<<"- L1 approx: "<<l1<<" MB/s\n";
        cout<<"- L2 approx: "<<l2<<" MB/s\n";
        cout<<"- L3 approx: "<<l3<<" MB/s\n";
        cout<<"- RAM: "<<ram<<" MB/s\n";
        cout<<"- Latency: "<<latency<<" ns\n\n";

        cout<<"# FINAL SCORE: "<<finalScore<<"\n";
        return 0;
    }

    /* -------- JSON -------- */

    if(json){
        cout<<"{\n";
        cout<<"  \"os\":\""<<os<<"\",\n";
        cout<<"  \"cpu\":\""<<cpu<<"\",\n";
	cout << "  \"ram_used_mb\": " << getRAMUsedMB() << ",\n"; 
       cout<<"  \"ram_mb\":"<<ramTotal<<",\n";
        cout<<"  \"integer\":"<<intScore<<",\n";
        cout<<"  \"float\":"<<floatScore<<",\n";
        cout<<"  \"avx2\":"<<avxScore<<",\n";
        cout<<"  \"multithread\":{\n";
        for(size_t i=0;i<levels.size();i++){
            cout<<"    \""<<levels[i]<<"\": "<<mtScores[i];
            if(i<levels.size()-1) cout<<",";
            cout<<"\n";
        }
        cout<<"  },\n";
        cout<<"  \"ram_bw\":"<<ram<<",\n";
        cout<<"  \"latency_ns\":"<<latency<<",\n";
        cout<<"  \"final_score\":"<<finalScore<<"\n";
        cout<<"}\n";
        return 0;
    }

    /* -------- TUI -------- */

    initscr(); noecho(); curs_set(0);

    mvprintw(1,2,"benchmark");
    mvprintw(3,2,"CPU int: %.2f",intScore);
    mvprintw(4,2,"CPU float: %.2f",floatScore);
    mvprintw(5,2,"CPU AVX2: %.2f",avxScore);

    int row=7;
    for(size_t i=0;i<levels.size();i++){
        mvprintw(row++,2,"Threads %d: %.2f",levels[i],mtScores[i]);
    }

    mvprintw(row+1,2,"RAM: %.2f MB/s",ram);
    mvprintw(row+2,2,"Latency: %.2f ns",latency);
    mvprintw(row+4,2,"FINAL SCORE: %.2f",finalScore);

    refresh();
    getch();
    endwin();

    return 0;
}
