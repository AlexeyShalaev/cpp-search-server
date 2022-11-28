// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
#include <bits/stdc++.h>

using namespace std;

int main() {
    int cnt = 0;
    for (int i = 1; i <= 1000; ++i) {
        if (to_string(i).find('3') != string::npos) {
            ++cnt;
        }
    }
    cout << cnt;
    return 0;
}