#include <irc/strbuf.h>
#include <check.h>
#include <stdio.h>

START_TEST(strbuf_test_new)
{
    strbuf_t b = strbuf_new();

    ck_assert(strbuf_len(b) == 0);
    ck_assert(strbuf_strdup(b) == NULL);
}
END_TEST

START_TEST(strbuf_test_append)
{
    strbuf_t b = strbuf_new();

    ck_assert(strbuf_append(b, "test\n", -1)
              == strlen("test\n")
        );
    ck_assert(strbuf_append(b, "foo\n", -1)
              == strlen("foo\n")
        );

    ck_assert(strbuf_len(b) == (strlen("foo\n") + strlen("test\n")));

    strbuf_free(b);
}
END_TEST

START_TEST(strbuf_test_getline)
{
    strbuf_t b = strbuf_new();
    char *line = NULL;
    size_t linelen = 0;

    strbuf_append(b, "test\n", -1);
    strbuf_append(b, "foo\n", -1);
    strbuf_append(b, "bar\n", -1);

    ck_assert(strbuf_getline(b, &line, &linelen) == 0);
    ck_assert(strcmp(line, "test\n") == 0);
    ck_assert(strbuf_len(b) == (strlen("foo\n") + strlen("bar\n")));
}
END_TEST

START_TEST(strbuf_test_getline_empty)
{
    strbuf_t b = strbuf_new();
    char *line = NULL;
    size_t linelen = 0;

    strbuf_append(b, "test\n", -1);
    strbuf_append(b, "foo\n", -1);
    strbuf_append(b, "bar\n", -1);

    ck_assert(strbuf_getline(b, &line, &linelen) == 0);
    ck_assert(strcmp(line, "test\n") == 0);
    ck_assert(strbuf_len(b) == (strlen("foo\n") + strlen("bar\n")));

    free(line);
    linelen = 0;

    ck_assert(strbuf_getline(b, &line, &linelen) == 0);
    ck_assert(strcmp(line, "foo\n") == 0);
    ck_assert(strbuf_len(b) == strlen("bar\n"));

    free(line);
    linelen = 0;

    ck_assert(strbuf_getline(b, &line, &linelen) == 0);
    ck_assert(strcmp(line, "bar\n") == 0);
    ck_assert(strbuf_len(b) == 0);

    free(line);
    linelen = 0;
}
END_TEST

START_TEST(strbuf_test_getstr)
{
    strbuf_t b = strbuf_new();
    char *line = NULL;
    size_t linelen = 0;

    strbuf_append(b, "test\r\n", -1);
    strbuf_append(b, "foo\r\n", -1);
    strbuf_append(b, "bar\r\n", -1);

    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    ck_assert(strcmp(line, "test\r\n") == 0);
    ck_assert(strbuf_len(b) == (strlen("foo\r\n") + strlen("bar\r\n")));
}
END_TEST

START_TEST(strbuf_test_getline_depleted)
{
    strbuf_t b = strbuf_new();
    char *line = NULL;
    size_t linelen = 0;

    strbuf_append(b, "test\n", -1);
    strbuf_append(b, "foo\n", -1);
    strbuf_append(b, "bar\n", -1);

    ck_assert(strbuf_getline(b, &line, &linelen) == 0);
    free(line);
    linelen = 0;
    ck_assert(strbuf_getline(b, &line, &linelen) == 0);
    free(line);
    linelen = 0;
    ck_assert(strbuf_getline(b, &line, &linelen) == 0);
    free(line);
    linelen = 0;

    /* no more data available
     */
    ck_assert(strbuf_getline(b, &line, &linelen) < 0);
}
END_TEST

START_TEST(strbuf_test_getstr_empty)
{
    strbuf_t b = strbuf_new();
    char *line = NULL;
    size_t linelen = 0;

    strbuf_append(b, "test\r\n", -1);
    strbuf_append(b, "foo\r\n", -1);
    strbuf_append(b, "bar\r\n", -1);

    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    ck_assert(strcmp(line, "test\r\n") == 0);
    ck_assert(strbuf_len(b) == (strlen("foo\r\n") + strlen("bar\r\n")));

    free(line);
    linelen = 0;

    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    ck_assert(strcmp(line, "foo\r\n") == 0);
    ck_assert(strbuf_len(b) == strlen("bar\r\n"));

    free(line);
    linelen = 0;

    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    ck_assert(strcmp(line, "bar\r\n") == 0);
    ck_assert(strbuf_len(b) == 0);

    free(line);
    linelen = 0;
}
END_TEST

START_TEST(strbuf_test_getstr_depleted)
{
    strbuf_t b = strbuf_new();
    char *line = NULL;
    size_t linelen = 0;

    strbuf_append(b, "test\r\n", -1);
    strbuf_append(b, "foo\r\n", -1);
    strbuf_append(b, "bar\r\n", -1);

    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    free(line);
    linelen = 0;
    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    free(line);
    linelen = 0;
    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    free(line);
    linelen = 0;

    /* no more data available
     */
    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") < 0);
}
END_TEST

START_TEST(strbuf_test_getstr_partial)
{
    strbuf_t b = strbuf_new();
    char *line = NULL;
    size_t linelen = 0;

    strbuf_append(b, "test\n", -1);
    strbuf_append(b, "foo\r\n", -1);
    strbuf_append(b, "bar\r\n", -1);
    strbuf_append(b, "meh", -1);

    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    ck_assert(strcmp(line, "test\nfoo\r\n") == 0);
    free(line);
    linelen = 0;
    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") == 0);
    ck_assert(strcmp(line, "bar\r\n") == 0);
    free(line);
    linelen = 0;
    ck_assert(strbuf_len(b) == strlen("meh"));

    /* no more data available
     */
    ck_assert(strbuf_getstr(b, &line, &linelen, "\r\n") < 0);
}
END_TEST

int main(int ac, char **av)
{
    Suite *s = NULL;
    SRunner *r = NULL;
    TCase *c = NULL;
    int failed = 0;

    /* Don't fork
     */
    putenv("CK_FORK=no");

    s = suite_create("strbuf");

    c = tcase_create("strbuf_new");
    tcase_add_test(c, strbuf_test_new);
    suite_add_tcase(s, c);

    c = tcase_create("strbuf_append");
    tcase_add_test(c, strbuf_test_append);
    suite_add_tcase(s, c);

    c = tcase_create("strbuf_getline");
    tcase_add_test(c, strbuf_test_getline);
    tcase_add_test(c, strbuf_test_getline_empty);
    tcase_add_test(c, strbuf_test_getline_depleted);
    suite_add_tcase(s, c);

    c = tcase_create("strbuf_getstr");
    tcase_add_test(c, strbuf_test_getstr);
    tcase_add_test(c, strbuf_test_getstr_empty);
    tcase_add_test(c, strbuf_test_getstr_depleted);
    tcase_add_test(c, strbuf_test_getstr_partial);
    suite_add_tcase(s, c);

    r = srunner_create(s);
    srunner_run_all(r, CK_NORMAL);
    failed = srunner_ntests_failed(r);

    srunner_free(r);

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
