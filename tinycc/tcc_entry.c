#include "../include/lib/mylibc.h"
int main(int argc, char **argv);
void _start_main(int argc, char **argv) {
    exit(main(argc, argv));
}
