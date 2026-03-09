#include <lib/mylibc.h>

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    // --- strlen ---
    printf("strlen(hello)=%d\n", (int)strlen("hello"));
    printf("strlen(\"\")=%d\n",  (int)strlen(""));

    // --- strcmp / strncmp ---
    printf("strcmp(abc,abc)=%d\n",      strcmp("abc", "abc"));
    printf("strcmp(abc<abd)=%d\n",      strcmp("abc", "abd") < 0 ? 1 : 0);
    printf("strcmp(abd>abc)=%d\n",      strcmp("abd", "abc") > 0 ? 1 : 0);
    printf("strncmp(abcX,abcY,3)=%d\n", strncmp("abcX", "abcY", 3));

    // --- strcpy / strcat ---
    char buf[64];
    strcpy(buf, "hello");
    strcat(buf, " world");
    printf("strcat: %s\n", buf);

    // --- strncpy ---
    char buf2[8];
    strncpy(buf2, "abcdefgh", 7);
    buf2[7] = '\0';
    printf("strncpy: %s\n", buf2);

    // --- strchr / strrchr ---
    char *p = strchr("hello", 'l');
    printf("strchr: %s\n",  p ? p : "null");
    p = strrchr("hello", 'l');
    printf("strrchr: %s\n", p ? p : "null");

    // --- strstr ---
    p = strstr("hello world", "world");
    printf("strstr: %s\n",      p ? p : "null");
    p = strstr("hello", "xyz");
    printf("strstr miss: %s\n", p ? p : "null");

    // --- memset / memcmp ---
    char mem[8];
    memset(mem, 0xAB, 4);
    printf("memcmp: %d\n", memcmp(mem, "\xAB\xAB\xAB\xAB", 4));

    // --- atoi / strtol ---
    printf("atoi(1234)=%d\n",   atoi("1234"));
    printf("atoi(-99)=%d\n",    atoi("-99"));
    printf("strtol(0xFF)=%d\n", (int)strtol("0xFF", NULL, 16));
    printf("strtol(010)=%d\n",  (int)strtol("010", NULL, 0));

    // --- 字符分类 ---
    printf("isdigit('5')=%d\n", isdigit('5'));
    printf("isalpha('z')=%d\n", isalpha('z'));
    printf("toupper('a')=%c\n", toupper('a'));
    printf("tolower('Z')=%c\n", tolower('Z'));

    // --- printf 格式 ---
    printf("[%5d]\n",  42);
    printf("[%-5d]\n", 42);
    printf("[%05d]\n", 42);
    printf("[%8s]\n",  "hi");
    printf("[%-8s]\n", "hi");
    printf("hex: %x\n", 255);
    printf("HEX: %X\n", 255);
    printf("oct: %o\n", 8);
    printf("ptr: %p\n", (void*)0x1234);

    // --- malloc / realloc ---
    char *m = (char*)malloc(16);
    strcpy(m, "malloc ok");
    printf("%s\n", m);
    m = (char*)realloc(m, 32);
    strcat(m, "!");
    printf("%s\n", m);

    // --- scanf %d ---
    printf("enter number: ");
    int n = 0;
    scanf("%d", &n);
    printf("got int: %d\n", n);

    // --- scanf %s ---
    printf("enter word: ");
    char word[32];
    scanf("%s", word);
    printf("got str: %s\n", word);

    // --- scanf %x ---
    printf("enter hex: ");
    unsigned int hval = 0;
    scanf("%x", &hval);
    printf("got hex: %x\n", hval);

    printf("done\n");
    return 0;
}