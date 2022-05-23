// Runs unit tests on arithmetic functions.
//
// Allocated heap memory for tests is never `free`d during the test execution,
// since this is meant to be a standalone executable. The allocated heap memory
// is at most some KB large.

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

#define TEST_SUCCESS()                  \
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
        printf(".");
        fflush(stdout);
    }
}

static void print_test_results() {
    printf(
        "\nfinished: %d test(s) -- %d successful -- %d failed\n",
        tests_run,
        tests_successful,
        tests_run - tests_successful
    );
}
static TestResult test_bn_compare() {
    BigNum *n1, *n2;

    n1 = bn_zero();
    n2 = bn_zero();
    TEST_ASSERT("0 == 0", bn_compare(n1, n2) == 0);
    TEST_ASSERT("0 == 0", bn_compare(n2, n1) == 0);

    n1 = bn_zero();
    n2 = bn_one();
    TEST_ASSERT("0 < 1", bn_compare(n1, n2) < 0);
    TEST_ASSERT("1 > 0", bn_compare(n2, n1) > 0);

    n1 = bn_from_hex("1 00000000");
    n2 = bn_from_hex("  FFFFFFFF");
    TEST_ASSERT("different lengths", bn_compare(n1, n2) > 0);
    TEST_ASSERT("different lengths", bn_compare(n2, n1) < 0);

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    TEST_ASSERT("", bn_compare(n1, n2) > 0);
    TEST_ASSERT("", bn_compare(n2, n1) < 0);

    n1 = bn_from_hex("1234567 89ABCDEF");
    n2 = bn_from_hex("1234567 89ABCDEF");
    TEST_ASSERT("", bn_compare(n1, n2) == 0);
    TEST_ASSERT("", bn_compare(n2, n1) == 0);

    TEST_SUCCESS();
}

static TestResult test_bn_greater_than() {
    BigNum *n1, *n2;

    n1 = bn_one();
    n2 = bn_zero();
    TEST_ASSERT("1 > 0", bn_greater_than(n1, n2));
    TEST_ASSERT("", !bn_greater_than(n2, n1));

    n1 = bn_from_hex("1 00000000");
    n2 = bn_from_hex("  FFFFFFFF");
    TEST_ASSERT("different lengths", bn_greater_than(n1, n2));
    TEST_ASSERT("different lengths", !bn_greater_than(n2, n1));

    n1 = bn_from_hex("2 00000000");
    n2 = bn_from_hex("1 FFFFFFFF");
    TEST_ASSERT("", bn_greater_than(n1, n2));
    TEST_ASSERT("", !bn_greater_than(n2, n1));

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("000AA213 F32785D1");
    TEST_ASSERT("", bn_greater_than(n1, n2));
    TEST_ASSERT("", !bn_greater_than(n2, n1));

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    TEST_ASSERT("", bn_greater_than(n1, n2));
    TEST_ASSERT("", !bn_greater_than(n2, n1));

    n1 = bn_from_hex("1234567 89ABCDEF");
    n2 = bn_from_hex("1234567 89ABCDEF");
    TEST_ASSERT("", !bn_greater_than(n1, n2));
    TEST_ASSERT("", !bn_greater_than(n2, n1));

    TEST_SUCCESS();
}

static TestResult test_bn_less_than() {
    BigNum *n1, *n2;

    n1 = bn_zero();
    n2 = bn_one();
    TEST_ASSERT("0 < 1", bn_less_than(n1, n2));
    TEST_ASSERT("not 1 < 0", !bn_less_than(n2, n1));

    n1 = bn_from_hex("  FFFFFFFF");
    n2 = bn_from_hex("1 00000000");
    TEST_ASSERT("", bn_less_than(n1, n2));
    TEST_ASSERT("", !bn_less_than(n2, n1));

    n1 = bn_from_hex("1 FFFFFFFF");
    n2 = bn_from_hex("2 00000000");
    TEST_ASSERT("", bn_less_than(n1, n2));
    TEST_ASSERT("", !bn_less_than(n2, n1));

    n1 = bn_from_hex("000AA213 F32785D1");
    n2 = bn_from_hex("EBA11829 27F45C1B");
    TEST_ASSERT("", bn_less_than(n1, n2));
    TEST_ASSERT("", !bn_less_than(n2, n1));

    n1 = bn_from_hex("CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    n2 = bn_from_hex("D1380128 25378933 47238921 10457832");
    TEST_ASSERT("", bn_less_than(n1, n2));
    TEST_ASSERT("", !bn_less_than(n2, n1));

    n1 = bn_from_hex("1234567 89ABCDEF");
    n2 = bn_from_hex("1234567 89ABCDEF");
    TEST_ASSERT("", !bn_less_than(n1, n2));
    TEST_ASSERT("", !bn_less_than(n2, n1));

    TEST_SUCCESS();
}

static TestResult test_bn_equal_to() {
    BigNum *n1, *n2;

    n1 = bn_zero();
    n2 = bn_zero();
    TEST_ASSERT("0 == 0", bn_equal_to(n1, n2));
    TEST_ASSERT("0 == 0", bn_equal_to(n2, n1));

    n1 = bn_zero();
    n2 = bn_one();
    TEST_ASSERT("0 != 1", !bn_equal_to(n1, n2));
    TEST_ASSERT("1 != 0", !bn_equal_to(n2, n1));

    n1 = bn_from_hex("1 00000000");
    n2 = bn_from_hex("  FFFFFFFF");
    TEST_ASSERT("different lengths", !bn_equal_to(n1, n2));
    TEST_ASSERT("different lengths", !bn_equal_to(n2, n1));

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    TEST_ASSERT("", !bn_equal_to(n1, n2));
    TEST_ASSERT("", !bn_equal_to(n2, n1));

    n1 = bn_from_hex("1234567 89ABCDEF");
    n2 = bn_from_hex("1234567 89ABCDEF");
    TEST_ASSERT("", bn_equal_to(n1, n2));
    TEST_ASSERT("", bn_equal_to(n2, n1));

    TEST_SUCCESS();
}


static TestResult test_bn_add() {
    BigNum *n1, *n2, *got_result, *should_result;

    n1 = bn_from_hex("FFFFFFFF");
    n2 = bn_from_hex("FFFFFFFF");
    got_result = bn_add(n1, n2);
    should_result = bn_from_hex("1 FFFFFFFE");
    TEST_ASSERT_EQ("can expand length", got_result, should_result);
    got_result = bn_add(n2, n1);
    TEST_ASSERT_EQ("can expand length", got_result, should_result);

    n2 = bn_zero();
    got_result = bn_add(n1, n2);
    should_result = n1;
    TEST_ASSERT_EQ("adding 0 does not affect first operand", got_result, should_result);
    got_result = bn_add(n2, n1);
    TEST_ASSERT_EQ("adding 0 does not affect first operand", got_result, should_result);

    n1 = bn_from_hex("AA213F 32785D1 FE1190ABB");
    n2 = bn_from_hex("       EBA11829 27F45C1B");
    got_result = bn_add(n1, n2);
    should_result = bn_from_hex("00AA2140 1E197549 090D66D6");
    TEST_ASSERT_EQ("", got_result, should_result);
    got_result = bn_add(n2, n1);
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("       EBA11829 27F45C1B");
    n2 = bn_from_hex("AA213F 32785D1F E1190ABB");
    got_result = bn_add(n1, n2);
    should_result = bn_from_hex("00AA2140 1E197549 090D66D6");
    TEST_ASSERT_EQ("", got_result, should_result);
    got_result = bn_add(n2, n1);
    TEST_ASSERT_EQ("", got_result, should_result);

    TEST_SUCCESS();
}

static TestResult test_bn_subtract() {
    BigNum *n1, *n2, *got_result, *should_result;

    n1 = bn_one();
    n2 = bn_zero();
    got_result = bn_subtract(n1, n2);
    should_result = n1;
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_one();
    n2 = bn_one();
    got_result = bn_subtract(n1, n2);
    should_result = bn_zero();
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("1 00000000");
    n2 = bn_from_hex("  FFFFFFFF");
    got_result = bn_subtract(n1, n2);
    should_result = bn_one();
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    got_result = bn_subtract(n1, n2);
    should_result = bn_from_hex("0288066B 2A499E57 9863DD31 55597978");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("AA213F 32785D1F E1190ABB");
    n2 = bn_from_hex("       EBA11829 27F45C1B");
    got_result = bn_subtract(n1, n2);
    should_result = bn_from_hex("00AA213E 46D744F6 B924AEA0");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("       EBA11829 27F45C1B");
    n2 = bn_from_hex("AA213F 32785D1F E1190ABB");
    got_result = bn_subtract(n1, n2);
    TEST_ASSERT("negative subtraction results in null pointer", !got_result);

    n1 = bn_zero();
    n2 = bn_one();
    got_result = bn_subtract(n1, n2);
    TEST_ASSERT("negative subtraction results in null pointer", !got_result);
    
    TEST_SUCCESS();
}

static TestResult test_bn_multiply() {
    BigNum *n1, *n2, *got_result, *should_result;

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_zero();
    got_result = bn_multiply(n1, n2);
    should_result = bn_zero();
    TEST_ASSERT_EQ("", got_result, should_result);
    got_result = bn_multiply(n2, n1);
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_one();
    got_result = bn_multiply(n1, n2);
    should_result = n1;
    TEST_ASSERT_EQ("", got_result, should_result);
    got_result = bn_multiply(n2, n1);
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("AA213F 32785D1F E1190ABB");
    n2 = bn_from_hex("       EBA11829 27F45C1B");
    got_result = bn_multiply(n1, n2);
    should_result = bn_from_hex("009C9793 FA8B087E B9811D85 0075E3B5 74BB55B9");
    TEST_ASSERT_EQ("", got_result, should_result);
    got_result = bn_multiply(n2, n1);
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    got_result = bn_multiply(n1, n2);
    should_result = bn_from_hex("A8EAE322 3BB9511C 9F5C249D 77A3CA8A A2A74A6F CD52AD21 1A7F75F2 69A0F054");
    TEST_ASSERT_EQ("", got_result, should_result);
    got_result = bn_multiply(n2, n1);
    TEST_ASSERT_EQ("", got_result, should_result);

    TEST_SUCCESS();
}

static TestResult test_bn_divide_with_remainder() {
    BigNum *n1, *n2, *should_quotient, *should_remainder;
    bn_DivideWithRemainderResult *got_result;

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_one();
    got_result = bn_divide_with_remainder(n1, n2);
    should_quotient = n1;
    should_remainder = bn_zero();
    TEST_ASSERT_EQ("", got_result->quotient, should_quotient);
    TEST_ASSERT_EQ("", got_result->remainder, should_remainder);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("EBA11829 27F45C1B");
    got_result = bn_divide_with_remainder(n1, n2);
    should_quotient = bn_one();
    should_remainder = bn_zero();
    TEST_ASSERT_EQ("", got_result->quotient, should_quotient);
    TEST_ASSERT_EQ("", got_result->remainder, should_remainder);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("     ACE 12791232");
    got_result = bn_divide_with_remainder(n1, n2);
    should_quotient = bn_from_hex("15CEB7");
    should_remainder = bn_from_hex("80E 08AA1E5D");
    TEST_ASSERT_EQ("", got_result->quotient, should_quotient);
    TEST_ASSERT_EQ("", got_result->remainder, should_remainder);

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("                  EBA11829 27F45C1B");
    got_result = bn_divide_with_remainder(n1, n2);
    should_quotient = bn_from_hex("E34E65BE ED03AB20");
    should_remainder = bn_from_hex("C477521F C4E2EBD2");
    TEST_ASSERT_EQ("", got_result->quotient, should_quotient);
    TEST_ASSERT_EQ("", got_result->remainder, should_remainder);

    n1 = bn_from_hex("AA213F 32785D1F E1190ABB");
    n2 = bn_from_hex("       129E781A C1829FB1");
    got_result = bn_divide_with_remainder(n1, n2);
    should_quotient = bn_from_hex("09232E2E");
    should_remainder = bn_from_hex("03F3ED12 94BB8AED");
    TEST_ASSERT_EQ("", got_result->quotient, should_quotient);
    TEST_ASSERT_EQ("", got_result->remainder, should_remainder);

    n1 = bn_from_hex("00AA213F 32785D1F E1190ABB");
    n2 = bn_from_hex("D1380128 CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    got_result = bn_divide_with_remainder(n1, n2);
    should_quotient = bn_zero();
    should_remainder = n1;
    TEST_ASSERT_EQ("", got_result->quotient, should_quotient);
    TEST_ASSERT_EQ("", got_result->remainder, should_remainder);

    n1 = bn_from_hex("C5367281 19283712");
    n2 = bn_from_hex("EBA11829 27F45C1B");
    got_result = bn_divide_with_remainder(n1, n2);
    should_quotient = bn_zero();
    should_remainder = n1;
    TEST_ASSERT_EQ("", got_result->quotient, should_quotient);
    TEST_ASSERT_EQ("", got_result->remainder, should_remainder);

    n1 = bn_from_hex("C5367281 19283712");
    n2 = bn_zero();
    got_result = bn_divide_with_remainder(n1, n2);
    TEST_ASSERT("dividing by zero results in null pointer", !got_result);

    TEST_SUCCESS();
}

static TestResult test_bn_divide() {
    BigNum *n1, *n2, *should_result, *got_result;

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_one();
    got_result = bn_divide(n1, n2);
    should_result = n1;
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("EBA11829 27F45C1B");
    got_result = bn_divide(n1, n2);
    should_result = bn_one();
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("     ACE 12791232");
    got_result = bn_divide(n1, n2);
    should_result = bn_from_hex("15CEB7");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("                  EBA11829 27F45C1B");
    got_result = bn_divide(n1, n2);
    should_result = bn_from_hex("E34E65BE ED03AB20");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("AA213F 32785D1F E1190ABB");
    n2 = bn_from_hex("       129E781A C1829FB1");
    got_result = bn_divide(n1, n2);
    should_result = bn_from_hex("09232E2E");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("C5367281 19283712");
    n2 = bn_zero();
    got_result = bn_divide(n1, n2);
    TEST_ASSERT("dividing by zero results in null pointer", !got_result);

    TEST_SUCCESS();
}

static TestResult test_bn_mod() {
    BigNum *n1, *n2, *got_result, *should_result;

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_one();
    got_result = bn_mod(n1, n2);
    should_result = bn_zero();
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("EBA11829 27F45C1B");
    got_result = bn_mod(n1, n2);
    should_result = bn_zero();
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("EBA11829 27F45C1B");
    n2 = bn_from_hex("     ACE 12791232");
    got_result = bn_mod(n1, n2);
    should_result = bn_from_hex("80E 08AA1E5D");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("D1380128 25378933 47238921 10457832");
    n2 = bn_from_hex("                  EBA11829 27F45C1B");
    got_result = bn_mod(n1, n2);
    should_result = bn_from_hex("C477521F C4E2EBD2");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("AA213F 32785D1F E1190ABB");
    n2 = bn_from_hex("       129E781A C1829FB1");
    got_result = bn_mod(n1, n2);
    should_result = bn_from_hex("03F3ED12 94BB8AED");
    TEST_ASSERT_EQ("", got_result, should_result);

    n1 = bn_from_hex("C5367281 19283712");
    n2 = bn_zero();
    got_result = bn_mod(n1, n2);
    TEST_ASSERT("modulo by zero results in null pointer", !got_result);

    TEST_SUCCESS();
}

static TestResult test_bn_power_mod() {
    BigNum *base, *exp, *mod, *got_result, *should_result;

    base = bn_zero();
    exp = bn_from_hex("10001");
    mod = bn_from_hex("ABCDABCD");
    got_result = bn_power_mod(base, exp, mod);
    should_result = bn_zero();
    TEST_ASSERT_EQ("base 0 results in 0", got_result, should_result);

    base = bn_from_hex("FFAABBEE CC115599");
    exp = bn_zero();
    mod = bn_from_hex("ABCDABCD");
    got_result = bn_power_mod(base, exp, mod);
    should_result = bn_one();
    TEST_ASSERT_EQ("exponent 0 results in 1", got_result, should_result);

    base = bn_from_hex("FFAABBEE CC115599");
    exp = bn_one();
    mod = bn_from_hex("ABCDABCD");
    got_result = bn_power_mod(base, exp, mod);
    should_result = bn_from_hex("30647631");
    TEST_ASSERT_EQ("exponent 0 calculates modulo of base", got_result, should_result);

    base = bn_from_hex("3");
    exp = bn_from_hex("10001");
    mod = bn_from_hex("ABCDABCD");
    got_result = bn_power_mod(base, exp, mod);
    should_result = bn_from_hex("5568556B");
    TEST_ASSERT_EQ("", got_result, should_result);

    base = bn_from_hex("D1380128 25378933 47238921 10457832");
    exp = bn_from_hex("1001");
    mod = bn_from_hex("CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    got_result = bn_power_mod(base, exp, mod);
    should_result = bn_from_hex("218CAD4A 31FC7FD4 2999356B 6BC523EA");
    TEST_ASSERT_EQ("", got_result, should_result);

    base = bn_from_hex("25378933 47238921 10457832");
    exp = bn_from_hex("FE21");
    mod = bn_from_hex("D1380128 CEAFFABC FAEDEADB AEBFABEF BAEBFEBA");
    got_result = bn_power_mod(base, exp, mod);
    should_result = bn_from_hex("004D4632 D1651F795 FE624A515 EE2CF5E0 095B4020");
    TEST_ASSERT_EQ("", got_result, should_result);

    base = bn_from_hex("25378933 47238921 10457832");
    exp = bn_from_hex("FE21");
    mod = bn_zero();
    got_result = bn_power_mod(base, exp, mod);
    TEST_ASSERT("`mod` = 0 results in null pointer", !got_result);

    TEST_SUCCESS();
}

int main(void) {
    run_test(test_bn_compare, "bn_compare");
    run_test(test_bn_greater_than, "bn_greater_than");
    run_test(test_bn_less_than, "bn_less_than");
    run_test(test_bn_equal_to, "bn_equal_to");
    run_test(test_bn_add, "bn_add");
    run_test(test_bn_subtract, "bn_subtract");
    run_test(test_bn_multiply, "bn_multiply");
    run_test(test_bn_divide_with_remainder, "bn_divide_with_remainder");
    run_test(test_bn_divide, "bn_divide");
    run_test(test_bn_mod, "bn_mod");
    run_test(test_bn_power_mod, "bn_power_mod");

    print_test_results();
    return !(tests_successful == tests_run);
}
