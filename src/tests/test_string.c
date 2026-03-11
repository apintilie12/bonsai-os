#include "tests/test_string.h"
#include "tests/test_common.h"
#include "lib/string.h"

// ----------------------------------------------------------------------------
// strlen
// ----------------------------------------------------------------------------
static void test_strlen(void) {
    LOG_CORE("-- strlen --\r\n");
    CHECK("empty string",        strlen("") == 0);
    CHECK("single char",         strlen("a") == 1);
    CHECK("normal string",       strlen("hello") == 5);
    CHECK("with spaces",         strlen("hi there") == 8);
}

// ----------------------------------------------------------------------------
// strcpy / strncpy
// ----------------------------------------------------------------------------
static void test_strcpy(void) {
    LOG_CORE("-- strcpy --\r\n");
    char buf[32];
    strcpy(buf, "hello");
    CHECK("basic copy",          strcmp(buf, "hello") == 0);
    strcpy(buf, "");
    CHECK("empty copy",          buf[0] == '\0');
}

static void test_strncpy(void) {
    LOG_CORE("-- strncpy --\r\n");
    char buf[8];
    // copy less than n: remainder must be zero-padded
    strncpy(buf, "hi", 8);
    CHECK("copy + zero pad",     buf[0]=='h' && buf[1]=='i' && buf[2]=='\0' && buf[7]=='\0');
    // copy exactly n chars from a longer string (no null terminator written)
    strncpy(buf, "abcdefgh", 4);
    CHECK("truncated copy",      buf[0]=='a' && buf[3]=='d');
}

// ----------------------------------------------------------------------------
// strcmp / strncmp
// ----------------------------------------------------------------------------
static void test_strcmp(void) {
    LOG_CORE("-- strcmp --\r\n");
    CHECK("equal strings",       strcmp("abc", "abc") == 0);
    CHECK("less than",           strcmp("abc", "abd") < 0);
    CHECK("greater than",        strcmp("abd", "abc") > 0);
    CHECK("prefix shorter",      strcmp("ab", "abc") < 0);
    CHECK("prefix longer",       strcmp("abc", "ab") > 0);
    CHECK("empty vs empty",      strcmp("", "") == 0);
}

static void test_strncmp(void) {
    LOG_CORE("-- strncmp --\r\n");
    CHECK("equal up to n",       strncmp("abcX", "abcY", 3) == 0);
    CHECK("differ within n",     strncmp("abcX", "abcY", 4) != 0);
    CHECK("n=0 always equal",    strncmp("abc", "xyz", 0) == 0);
}

// ----------------------------------------------------------------------------
// strchr / strrchr
// ----------------------------------------------------------------------------
static void test_strchr(void) {
    LOG_CORE("-- strchr --\r\n");
    const char *s = "hello";
    CHECK("found first",         strchr(s, 'l') == s + 2);
    CHECK("not found",           strchr(s, 'z') == 0);
    CHECK("find null term",      strchr(s, '\0') == s + 5);
}

static void test_strrchr(void) {
    LOG_CORE("-- strrchr --\r\n");
    const char *s = "hello";
    CHECK("found last",          strrchr(s, 'l') == s + 3);
    CHECK("not found",           strrchr(s, 'z') == 0);
    CHECK("only occurrence",     strrchr(s, 'h') == s);
}

// ----------------------------------------------------------------------------
// strtok_r
// ----------------------------------------------------------------------------
static void test_strtok_r(void) {
    LOG_CORE("-- strtok_r --\r\n");
    char buf[32];
    char *save;
    char *tok;

    // basic split
    strcpy(buf, "one two three");
    tok = strtok_r(buf, " ", &save);
    CHECK("token 1",             tok && strcmp(tok, "one") == 0);
    tok = strtok_r(0, " ", &save);
    CHECK("token 2",             tok && strcmp(tok, "two") == 0);
    tok = strtok_r(0, " ", &save);
    CHECK("token 3",             tok && strcmp(tok, "three") == 0);
    tok = strtok_r(0, " ", &save);
    CHECK("exhausted",           tok == 0);

    // leading/trailing delimiters
    strcpy(buf, "/path/to/file");
    tok = strtok_r(buf, "/", &save);
    CHECK("path token 1",        tok && strcmp(tok, "path") == 0);
    tok = strtok_r(0, "/", &save);
    CHECK("path token 2",        tok && strcmp(tok, "to") == 0);
    tok = strtok_r(0, "/", &save);
    CHECK("path token 3",        tok && strcmp(tok, "file") == 0);

    // single token, no delimiter
    strcpy(buf, "alone");
    tok = strtok_r(buf, " ", &save);
    CHECK("single token",        tok && strcmp(tok, "alone") == 0);
    tok = strtok_r(0, " ", &save);
    CHECK("single then null",    tok == 0);
}

// ----------------------------------------------------------------------------
// memset
// ----------------------------------------------------------------------------
static void test_memset(void) {
    LOG_CORE("-- memset --\r\n");
    char buf[8];
    memset(buf, 0, 8);
    CHECK("zero fill",           buf[0]==0 && buf[7]==0);
    memset(buf, 0xFF, 4);
    CHECK("pattern fill",        (unsigned char)buf[0]==0xFF && (unsigned char)buf[3]==0xFF);
    CHECK("unchanged tail",      buf[4]==0 && buf[7]==0);
}

// ----------------------------------------------------------------------------
// toupper / tolower
// ----------------------------------------------------------------------------
static void test_case(void) {
    LOG_CORE("-- toupper/tolower --\r\n");
    CHECK("toupper a-z",         toupper('a') == 'A' && toupper('z') == 'Z');
    CHECK("toupper unchanged",   toupper('A') == 'A' && toupper('0') == '0');
    CHECK("tolower A-Z",         tolower('A') == 'a' && tolower('Z') == 'z');
    CHECK("tolower unchanged",   tolower('a') == 'a' && tolower('9') == '9');
}

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------
void test_string(void) {
    LOG_CORE("=== string tests ===\r\n");
    test_strlen();
    test_strcpy();
    test_strncpy();
    test_strcmp();
    test_strncmp();
    test_strchr();
    test_strrchr();
    test_strtok_r();
    test_memset();
    test_case();
    LOG_CORE("=== string tests done ===\r\n");
}
