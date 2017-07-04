#include <check.h>
#include <stdlib.h>

#include "../src/daemonize.h"
#include "daemonize_test.h"

START_TEST(a_test)
{
    ck_assert(0);
}
END_TEST

Suite *make_daemonize_test_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("Daemonize");
    tc = tcase_create("Core");

    tcase_add_test(tc, a_test);

    suite_add_tcase(s, tc);

    return s;
}
