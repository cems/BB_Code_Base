#include <stdio.h>

void main() {
    system("./serial -p ttyS2 | ./sender host01 10.20.1.1");
}
