#include "../map.hpp"
#include <cstdio>

int main() {
    sjtu::map<int, int> map;
    for (int i = 1; i <= 10000; i++) map[i];
    map.__right_rotate(map.root);
    return 0;
}