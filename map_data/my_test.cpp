#include "../map.hpp"
#include <cstdio>

int main() {
    sjtu::map<int, int> map;
    for (int i = 1; i <= 200000; i++) {
        map[rand() % 200000] = rand();
        auto it = map.find(rand() % 200000);
        if (it != map.end()) map.erase(it);
    }
    auto it = map.begin();
    for (int i = 1; i <= 10000000; i++) {
        if (it == map.end()) it--;
        else if (it == map.begin()) it++;
        else if (rand() % 2) it++;
        else it--;
    }
}
