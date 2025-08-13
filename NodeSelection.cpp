// g++ -O3 -std=c++17 NodeSelectionSimple.cpp -fopenmp -o NodeSelectionSimple

#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <utility>
#include <chrono>
#include <omp.h>

using namespace std;

struct node {
    int id;
    int num;

    node() {}

    node(int id, int num) : id(id), num(num) {}
};

class MyPQ {
public:
    MyPQ(int n) {
        elements.reserve(n);
    }

    void MakeHeap() {
        int n = elements.size();
        for (int i=0;i<n;i++) {
            ID2Index[elements[i].id] = i;
        }

        for (int i=(n-2)/2;i>=0;i--) {
            Down(i);
        }
    }

    void Down(int index) {
        int n = elements.size();

        while (2 * index + 1 < n) {
            int child = 2 * index + 1;

            if (child + 1 < n && compare(elements[child], elements[child + 1])) {
                child++;
            }

            if (compare(elements[index], elements[child])) {
                ID2Index[elements[index].id] = child;
                ID2Index[elements[child].id] = index;
                swap(elements[index], elements[child]);
                index = child;
            } else {
                break;
            }
        }
    }

    void Delete(int id) {
        if (elements.size() == 1) {
            elements.pop_back();
            return;
        }

        int index = ID2Index[id];
        elements[index] = elements[elements.size()-1]; // swap with the last one
        ID2Index[elements[index].id] = index;
        ID2Index.erase(id);
        elements.pop_back();
        Down(index);
    }

    void Dec(int id, int decNum) {
        int index = ID2Index[id];
        elements[index].num -= decNum;
        Down(index);
    }

    static bool compare(const node& p1, const node& p2) {
        return p1.num < p2.num;
    }

public:
    unordered_map<int, int> ID2Index;
    vector<node> elements;
};



int n;
vector<vector<int>> graph;

vector<int> isCovered;
vector<int> anchor;

void FindKHopGraph(int id, int c, vector<int>& visit, vector<int>& visitedNode) {
    visitedNode.push_back(id);
    visit[id] = 1;

    set<int> s = {id};
    for (int i=0;i<c;i++) {
        set<int> tmpS;
        for (auto u : s) {
            for (auto v : graph[u]) {
                if (!visit[v]) {
                    visit[v] = 1;
                    visitedNode.push_back(v);
                    tmpS.emplace(v);
                }
            }
        }
        swap(s, tmpS);
    }
}

int main(int argc, char* argv[]) {
    {
        string fileName(argv[1]);

        ifstream f(fileName);
        int u, v, t;
        
        while (f >> u >> v) {
            t = u > v ? u : v;
            if (t + 1 > graph.size()) {
                graph.resize(t + 1);
            }

            graph[u].emplace_back(v);
            graph[v].emplace_back(u);
        }
    }

    n = graph.size();

    int c = atoi(argv[2]);
    double cr = atof(argv[3]);

    omp_set_num_threads(64);

    auto start = std::chrono::high_resolution_clock::now();

    isCovered.resize(n, 0);
    MyPQ pq = MyPQ(n);
    
    // heap init
    {
        int id = 0;
        #pragma omp parallel private(id)
        {
            vector<int> visit(n, 0);

            #pragma omp for schedule(dynamic, 1) nowait
            for (id=0;id<n;++id) {
                // build c-hop graph
                vector<int> visitedNode;
                FindKHopGraph(id, c, visit, visitedNode);

                for (int i : visitedNode) {
                    visit[i] = 0;
                }

                #pragma omp critical
                {
                    pq.elements.emplace_back(id, visitedNode.size());
                }
            }
        }

        pq.MakeHeap();
    }

    {
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        cout << "Heap Init Time(ms): " << diff.count() / 1000000 << endl;
    }

    vector<int> visit(n, 0);

    while (pq.elements.size() >= (1 - cr) * n) {
        node top = pq.elements[0];
        int id = top.id;
        anchor.push_back(id);
        // cout << id << " " << top.num << endl;

        vector<int> visitedNode; // covered nodes + uncovered nodes
        FindKHopGraph(id, c, visit, visitedNode);

        vector<int> affect; // newly covered nodes, including itself
        for (auto i : visitedNode) {
            if (!isCovered[i]) {
                affect.push_back(i);
            }
        }

        unordered_map<int, int> decNodes;

        // prepare decNodes
        int i = 1;
        #pragma omp parallel private(i)
        {
            vector<int> localVisit(n, 0);

            #pragma omp for schedule(dynamic, 1) nowait
            for (i=1;i<affect.size();i++) { // omit itself
                int id = visitedNode[i];

                vector<int> localVisitedNode;
                vector<int> exist;

                FindKHopGraph(id, c, localVisit, localVisitedNode);

                for (int i : localVisitedNode) {
                    if (!visit[i] && !isCovered[i]) {
                        exist.push_back(i);
                    }

                    localVisit[i] = 0;
                }

                #pragma omp critical
                {
                    for (auto i : exist) {
                        decNodes[i]++;
                    }
                }
            }
        }

        // delete nodes
        for (int i : affect) {
            pq.Delete(i);
            isCovered[i] = 1;
        }

        for (int i : visitedNode) {
            visit[i] = 0;
        }

        // update existing nodes
        for (auto [id, decNum] : decNodes) {
            pq.Dec(id, decNum);
        }

        // for (auto u : visitedNode) {
        //     for (auto v : graph[u]) {
        //         graph[v].erase(u);
        //     }
        //     graph[u].clear();
        // }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    cout << "Time(ms): " << diff.count() / 1000000 << endl;
    // cout << "Time(ns): " << diff.count() << endl;
    cout << "c: " << c << " cr: 0." << int(cr * 10) << endl;
    cout << "n: " << n << "  anchor: " << anchor.size() << " coverNodeNum: " << n - pq.elements.size() << endl;


    if (atoi(argv[4]) == 1) {

        vector<vector<int>> embed(n, vector<int>(anchor.size()));

        int INF = 9999;

        auto start = std::chrono::high_resolution_clock::now();

        #pragma omp parallel
        {
            #pragma omp for schedule(dynamic, 1) nowait
            for (auto index=0;index<anchor.size();++index) {
                int anchorNode = anchor[index];

                vector<int> dis(n, INF);
                dis[anchorNode] = 0;

                auto pq = priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>>();
                pq.emplace(0, anchorNode);

                while (!pq.empty()) {
                    auto [curDis, u] = pq.top();
                    pq.pop();

                    if (curDis > dis[u]) {
                        continue;
                    }

                    for (auto v : graph[u]) {
                        if (curDis + 1 < dis[v]) {
                            dis[v] = curDis + 1;
                            pq.emplace(curDis + 1, v);
                        }
                    }
                }

                #pragma omp critical
                {
                    for (int i=0;i<n;i++) {
                        embed[i][index] = dis[i];
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        cout << "Embed Gen Time(ms): " << diff.count() / 1000000 << endl;


        string fileName(argv[1]);
        string outputFile = fileName + ".embed_" + to_string(c) + "_0." + to_string(int(10*cr));
        ofstream f(outputFile);
        for (int i=0;i<n;i++) {
            for (auto j : embed[i]) {
                f << j << " ";
            }
            f << endl;
        }

        string outputFileNode = fileName + ".embed.node_" + to_string(c) + "_0." + to_string(int(10*cr));
        ofstream fNode(outputFileNode);
        for (auto i : anchor) {
            fNode << i << endl;;
        }
    }
}

