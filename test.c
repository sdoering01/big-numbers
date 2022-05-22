#include "arithmetic.c"

typedef struct TestResult {
    int success;
    int line;
    char *message;
} TestResult;

#define TEST_ASSERT(message, test)                          \
    do {                                                    \
        if (!(test)) {                                      \
            TestResult result = { 0, __LINE__, message };   \
            return result;                                  \
        }                                                   \
    } while (0)

#define TEST_ASSERT_EQ(message, n1, n2) TEST_ASSERT(message, bn_equal_to(n1, n2))

#define SUCCESS()                       \
    do {                                \
        TestResult r = {1, 0, NULL};    \
        return r;                       \
    } while (0)

int tests_run = 0;
int tests_successful = 0;

static void run_test(TestResult (*test)(), const char *test_name) {
    TestResult r = test();
    tests_run++;
    if (!r.success) {
        if (r.message && *r.message) {
            fprintf(stderr, "%s failed (assertion in line %d): %s\n", test_name, r.line, r.message);
        } else {
            fprintf(stderr, "%s failed (line %d)\n", test_name, r.line);
        }
    } else {
        tests_successful++;
    }
}

static void print_test_results() {
    printf(
        "finished: %d test(s) run -- %d test(s) successful -- %d test(s) failed\n",
        tests_run,
        tests_successful,
        tests_run - tests_successful
    );
}

static TestResult test_bn_add() {
    BigNum *n1, *n2, *has_result, *should_result;

    n1 = bn_from_hex("FFFFFFFF");
    n2 = bn_from_hex("FFFFFFFF");
    has_result = bn_add(n1, n2);
    should_result = bn_from_hex("1 FFFFFFFE");
    TEST_ASSERT_EQ("can expand length", has_result, should_result);

    n2 = bn_from_uint32_t(0);
    has_result = bn_add(n1, n2);
    should_result = n1;
    TEST_ASSERT_EQ("adding 0 does not affect first operand", has_result, should_result);

    n1 = bn_from_hex("000AA213 F32785D1 FE1190ABB");
    n2 = bn_from_hex("EBA11829 27F45C1B");
    has_result = bn_add(n1, n2);
    should_result = bn_from_hex("00AA2140 1E197549 090D66D6");
    TEST_ASSERT_EQ("", has_result, should_result);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("000AA213 F32785D1 FE1190ABB");
    has_result = bn_add(n1, n2);
    should_result = bn_from_hex("00AA2140 1E197549 090D66D6");
    TEST_ASSERT_EQ("", has_result, should_result);

    SUCCESS();
}

int main(void) {
    run_test(test_bn_add, "bn_add");

    print_test_results();
    return 0;
}
