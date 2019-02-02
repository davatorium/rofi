/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <assert.h>
#include <locale.h>
#include <glib.h>
#include <stdio.h>
#include <helper.h>
#include <string.h>
#include <xcb/xcb_ewmh.h>
#include "display.h"
#include "xcb.h"
#include "xcb-internal.h"
#include "rofi.h"
#include "settings.h"
#include "rofi-types.h"

#include <check.h>

void rofi_add_error_message ( G_GNUC_UNUSED GString *msg )
{
}
int rofi_view_error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
    return TRUE;
}
int monitor_active ( G_GNUC_UNUSED workarea *mon )
{
    return 0;
}

void display_startup_notification ( G_GNUC_UNUSED RofiHelperExecuteContext *context, G_GNUC_UNUSED GSpawnChildSetupFunc *child_setup, G_GNUC_UNUSED gpointer *user_data )
{
}
START_TEST(test_tokenizer_free )
{
        helper_tokenize_free ( NULL );
}
END_TEST
START_TEST ( test_tokenizer_match_normal_single_ci )
{
    config.matching_method = MM_NORMAL;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", FALSE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , TRUE );

    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_normal_single_cs )
{
    config.matching_method = MM_NORMAL;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", TRUE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , FALSE );

    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_normal_multiple_ci )
{
    config.matching_method = MM_NORMAL;
    rofi_int_matcher **tokens = helper_tokenize ( "no ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE );
    helper_tokenize_free ( tokens );

}
END_TEST

START_TEST ( test_tokenizer_match_normal_single_ci_negate )
{
    config.matching_method = MM_NORMAL;
    rofi_int_matcher **tokens = helper_tokenize ( "-noot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE );
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_normal_multiple_ci_negate )
{
    config.matching_method = MM_NORMAL;
    rofi_int_matcher **tokens = helper_tokenize ( "-noot aap", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , FALSE );
    helper_tokenize_free ( tokens );

}
END_TEST

START_TEST ( test_tokenizer_match_glob_single_ci )
{
    config.matching_method = MM_GLOB;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", FALSE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , TRUE );

    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_glob_single_cs )
{
    config.matching_method = MM_GLOB;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", TRUE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , FALSE );
    helper_tokenize_free ( tokens );

}
END_TEST

START_TEST ( test_tokenizer_match_glob_multiple_ci )
{
    config.matching_method = MM_GLOB;
    rofi_int_matcher **tokens = helper_tokenize ( "no ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE );
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_glob_single_ci_question )
{
    config.matching_method = MM_GLOB;
    rofi_int_matcher **tokens = helper_tokenize ( "n?ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , FALSE);
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_glob_single_ci_star )
{
    config.matching_method = MM_GLOB;
    rofi_int_matcher **tokens = helper_tokenize ( "n*ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE);
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_glob_multiple_ci_star )
{
    config.matching_method = MM_GLOB;
    rofi_int_matcher **tokens = helper_tokenize ( "n* ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE);
    ck_assert_int_eq ( helper_token_match ( tokens, "ot nap mies") , TRUE);
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_fuzzy_single_ci )
{
    config.matching_method = MM_FUZZY;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", FALSE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , TRUE );

    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_fuzzy_single_cs )
{
    config.matching_method = MM_FUZZY;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", TRUE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , FALSE );
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_fuzzy_multiple_ci )
{
    config.matching_method = MM_FUZZY;
    rofi_int_matcher **tokens = helper_tokenize ( "no ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE );
    helper_tokenize_free ( tokens );

    tokens = helper_tokenize ( "n ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE);
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_fuzzy_single_ci_split )
{
    config.matching_method = MM_FUZZY;
    rofi_int_matcher **tokens = helper_tokenize ( "ont", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , FALSE);
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap nmiest") , TRUE );
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_fuzzy_multiple_ci_split )
{
    config.matching_method = MM_FUZZY;
    rofi_int_matcher **tokens = helper_tokenize ( "o n t", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE);
    ck_assert_int_eq ( helper_token_match ( tokens, "ot nap mies") , TRUE);
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_regex_single_ci )
{
    config.matching_method = MM_REGEX;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", FALSE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , TRUE );

    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_regex_single_cs )
{
    config.matching_method = MM_REGEX;
    rofi_int_matcher **tokens = helper_tokenize ( "noot", TRUE );

    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap Noot mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "Nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noOTap mies") , FALSE );
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_regex_multiple_ci )
{
    config.matching_method = MM_REGEX;
    rofi_int_matcher **tokens = helper_tokenize ( "no ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE );
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_regex_single_ci_dq )
{
    config.matching_method = MM_REGEX;
    rofi_int_matcher **tokens = helper_tokenize ( "n.?ot", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , FALSE);
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_regex_single_two_char )
{
    config.matching_method = MM_REGEX;
    rofi_int_matcher **tokens = helper_tokenize ( "n[oa]{2}t", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , TRUE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noat miesot") , TRUE);
    ck_assert_int_eq ( helper_token_match ( tokens, "noaat miesot") , FALSE);
    helper_tokenize_free ( tokens );
}
END_TEST

START_TEST ( test_tokenizer_match_regex_single_two_word_till_end )
{
    config.matching_method = MM_REGEX;
    rofi_int_matcher **tokens = helper_tokenize ( "^(aap|noap)\\sMie.*", FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap noot mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "aap mies") , TRUE);
    ck_assert_int_eq ( helper_token_match ( tokens, "nooaap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "nootap mies") , FALSE );
    ck_assert_int_eq ( helper_token_match ( tokens, "noap miesot") , TRUE);
    ck_assert_int_eq ( helper_token_match ( tokens, "ot nap mies") , FALSE );
    helper_tokenize_free ( tokens );
}
END_TEST

static Suite * helper_tokenizer_suite (void)
{
    Suite *s;

    s = suite_create("Tokenizer");

    /* Core test case */
    {
        TCase *tc_core;
        tc_core = tcase_create("Core");
        tcase_add_test(tc_core, test_tokenizer_free);
        suite_add_tcase(s, tc_core);
    }
    {
        TCase *tc_normal = tcase_create ("Normal");
        tcase_add_test(tc_normal, test_tokenizer_match_normal_single_ci );
        tcase_add_test(tc_normal, test_tokenizer_match_normal_single_cs );
        tcase_add_test(tc_normal, test_tokenizer_match_normal_multiple_ci );
        tcase_add_test(tc_normal, test_tokenizer_match_normal_single_ci_negate );
        tcase_add_test(tc_normal, test_tokenizer_match_normal_multiple_ci_negate);
        suite_add_tcase(s, tc_normal);
    }
    {
        TCase *tc_glob = tcase_create ("Glob");
        tcase_add_test(tc_glob, test_tokenizer_match_glob_single_ci);
        tcase_add_test(tc_glob, test_tokenizer_match_glob_single_cs);
        tcase_add_test(tc_glob, test_tokenizer_match_glob_multiple_ci);
        tcase_add_test(tc_glob, test_tokenizer_match_glob_single_ci_question);
        tcase_add_test(tc_glob, test_tokenizer_match_glob_single_ci_star);
        tcase_add_test(tc_glob, test_tokenizer_match_glob_multiple_ci_star);
        suite_add_tcase(s, tc_glob);
    }
    {
        TCase *tc_fuzzy = tcase_create ("Fuzzy");
        tcase_add_test(tc_fuzzy, test_tokenizer_match_fuzzy_single_ci);
        tcase_add_test(tc_fuzzy, test_tokenizer_match_fuzzy_single_cs);
        tcase_add_test(tc_fuzzy, test_tokenizer_match_fuzzy_single_ci_split);
        tcase_add_test(tc_fuzzy, test_tokenizer_match_fuzzy_multiple_ci);
        tcase_add_test(tc_fuzzy, test_tokenizer_match_fuzzy_multiple_ci_split);
        suite_add_tcase(s, tc_fuzzy);
    }
    {
        TCase *tc_regex = tcase_create ("Regex");
        tcase_add_test(tc_regex, test_tokenizer_match_regex_single_ci);
        tcase_add_test(tc_regex, test_tokenizer_match_regex_single_cs);
        tcase_add_test(tc_regex, test_tokenizer_match_regex_single_ci_dq);
        tcase_add_test(tc_regex, test_tokenizer_match_regex_single_two_char);
        tcase_add_test(tc_regex, test_tokenizer_match_regex_single_two_word_till_end);
        tcase_add_test(tc_regex, test_tokenizer_match_regex_multiple_ci);
        suite_add_tcase(s, tc_regex);
    }


    return s;
}

int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char ** argv )
{
    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }

    int number_failed = 0;
    Suite *s;
    SRunner *sr;

    s = helper_tokenizer_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    
}
