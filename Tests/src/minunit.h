/*
 * Copyright (c) 2012 David Siñuela Pastor, siu.4coders@gmail.com
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef MINUNIT_MINUNIT_H
#define MINUNIT_MINUNIT_H

#ifdef __cplusplus

#include <srl_log.hpp>

using namespace SRL::Logger;

extern "C"
{
#endif

#if __GNUC__ >= 5 && !defined(__STDC_VERSION__)
#define __func__ __extension__ __FUNCTION__
#endif

#include <string.h>
#include <math.h>

/*  Maximum length of last message */
#define MINUNIT_MESSAGE_LEN 1024
/*  Accuracy with which floats are compared */
#define MINUNIT_EPSILON 1E-12

	/*  Misc. counters */
	static int minunit_run = 0;
	static int minunit_assert = 0;
	static int minunit_fail = 0;
	static int minunit_status = 0;
	static uint32_t suite_error_counter = 0;

	/*  Timers */
	static double minunit_real_timer = 0;
	static double minunit_proc_timer = 0;

	/*  Last message */
	static char minunit_last_message[MINUNIT_MESSAGE_LEN];

	/*  Test setup and teardown function pointers */
	static void (*minunit_setup)(void) = NULL;
	static void (*minunit_teardown)(void) = NULL;
	static void (*minunit_output_header)(void) = NULL;

/*  Definitions */
#define MU_TEST(method_name) static void method_name(void)
#define MU_TEST_SUITE(suite_name) static void suite_name(void)

#define MU__SAFE_BLOCK(block) \
	do                        \
	{                         \
		block                 \
	} while (0)

/*  Run test suite and unset setup and teardown functions */
#define MU_RUN_SUITE(suite_name) MU__SAFE_BLOCK( \
	suite_name();                                \
	minunit_setup = NULL;                        \
	minunit_teardown = NULL;                     \
	minunit_output_header = NULL;)

/*  Configure setup and teardown functions */
#define MU_SUITE_CONFIGURE(setup_fun, teardown_fun) MU__SAFE_BLOCK( \
	suite_error_counter = 0;                                        \
	minunit_setup = setup_fun;                                      \
	minunit_teardown = teardown_fun;                                \
	minunit_output_header = NULL;)

/*  Configure setup and teardown functions */
#define MU_SUITE_CONFIGURE_WITH_HEADER(setup_fun, teardown_fun, output_header) MU__SAFE_BLOCK( \
	suite_error_counter = 0;                                                                   \
	minunit_setup = setup_fun;                                                                 \
	minunit_teardown = teardown_fun;                                                           \
	minunit_output_header = output_header;)

/*  Test runner */
#define MU_RUN_TEST(test) MU__SAFE_BLOCK(                            \
	if (minunit_real_timer == 0 && minunit_proc_timer == 0) {        \
		minunit_real_timer = mu_timer_real();                        \
		minunit_proc_timer = mu_timer_cpu();                         \
	} if (minunit_setup) (*minunit_setup)();                         \
	minunit_status = 0;                                              \
	test();                                                          \
	++minunit_run;                                                   \
	if (minunit_output_header)                                   	 \
			(*minunit_output_header)();  							 \
	if (minunit_status) {                                            \
		++minunit_fail;												 \
		Log::LogPrint<LogLevels::FATAL>("%s", minunit_last_message); \
	} else { 														 \
		Log::LogPrint<LogLevels::TESTING>("Passed :%s", #test);		 \
	} if (minunit_teardown)(*minunit_teardown)();)					

/*  Report */
#define MU_REPORT() MU__SAFE_BLOCK(                                                                                    \
	double minunit_end_real_timer;                                                                                     \
	double minunit_end_proc_timer;                                                                                     \
	Log::LogPrint<LogLevels::INFO>("%d tests, %d assertions, %d failures", minunit_run, minunit_assert, minunit_fail); \
	minunit_end_real_timer = mu_timer_real();                                                                          \
	minunit_end_proc_timer = mu_timer_cpu();                                                                           \
	Log::LogPrint<LogLevels::INFO>("Finished in %.8f seconds (real) %.8f seconds (proc)",                              \
								   minunit_end_real_timer - minunit_real_timer,                                        \
								   minunit_end_proc_timer - minunit_proc_timer);)
#define MU_EXIT_CODE minunit_fail

/*  Assertions */
#define mu_check(test) MU__SAFE_BLOCK(                                                                                             \
	minunit_assert++;                                                                                                              \
	if (!(test)) {                                                                                                                 \
		(void)snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, #test); \
		minunit_status = 1;                                                                                                        \
		return;                                                                                                                    \
	})

#define mu_fail(message) MU__SAFE_BLOCK(                                                                                         \
	minunit_assert++;                                                                                                            \
	(void)snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, message); \
	minunit_status = 1;                                                                                                          \
	return;)

#define mu_assert(test, message) MU__SAFE_BLOCK(                                                                                     \
	minunit_assert++;                                                                                                                \
	if (!(test)) {                                                                                                                   \
		(void)snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, message); \
		minunit_status = 1;                                                                                                          \
		return;                                                                                                                      \
	})

#define mu_assert_int_eq(expected, result) MU__SAFE_BLOCK(                                                                                                                    \
	int minunit_tmp_e;                                                                                                                                                        \
	int minunit_tmp_r;                                                                                                                                                        \
	minunit_assert++;                                                                                                                                                         \
	minunit_tmp_e = (expected);                                                                                                                                               \
	minunit_tmp_r = (result);                                                                                                                                                 \
	if (minunit_tmp_e != minunit_tmp_r) {                                                                                                                                     \
		(void)snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %d expected but was %d", __func__, __FILE__, __LINE__, minunit_tmp_e, minunit_tmp_r); \
		minunit_status = 1;                                                                                                                                                   \
		return;                                                                                                                                                               \
	})

#define mu_assert_double_eq(expected, result) MU__SAFE_BLOCK(                                                                                                                                                                               \
	double minunit_tmp_e;                                                                                                                                                                                                                   \
	double minunit_tmp_r;                                                                                                                                                                                                                   \
	minunit_assert++;                                                                                                                                                                                                                       \
	minunit_tmp_e = (expected);                                                                                                                                                                                                             \
	minunit_tmp_r = (result);                                                                                                                                                                                                               \
	if (fabs(minunit_tmp_e - minunit_tmp_r) > MINUNIT_EPSILON) {                                                                                                                                                                            \
		int minunit_significant_figures = 1 - log10(MINUNIT_EPSILON);                                                                                                                                                                       \
		(void)snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %.*g expected but was %.*g", __func__, __FILE__, __LINE__, minunit_significant_figures, minunit_tmp_e, minunit_significant_figures, minunit_tmp_r); \
		minunit_status = 1;                                                                                                                                                                                                                 \
		return;                                                                                                                                                                                                                             \
	})

#define mu_assert_string_eq(expected, result) MU__SAFE_BLOCK(                                                                                                                     \
	const char *minunit_tmp_e = expected;                                                                                                                                         \
	const char *minunit_tmp_r = result;                                                                                                                                           \
	minunit_assert++;                                                                                                                                                             \
	if (!minunit_tmp_e) {                                                                                                                                                         \
		minunit_tmp_e = "<null pointer>";                                                                                                                                         \
	} if (!minunit_tmp_r) {                                                                                                                                                       \
		minunit_tmp_r = "<null pointer>";                                                                                                                                         \
	} if (strcmp(minunit_tmp_e, minunit_tmp_r) != 0) {                                                                                                                            \
		(void)snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: '%s' expected but was '%s'", __func__, __FILE__, __LINE__, minunit_tmp_e, minunit_tmp_r); \
		minunit_status = 1;                                                                                                                                                       \
		return;                                                                                                                                                                   \
	})

	/*
	 * The following two functions were written by David Robert Nadeau
	 * from http://NadeauSoftware.com/ and distributed under the
	 * Creative Commons Attribution 3.0 Unported License
	 */

	/**
	 * Returns the real time, in seconds, or -1.0 if an error occurred.
	 *
	 * Time is measured since an arbitrary and OS-dependent start time.
	 * The returned real time is only useful for computing an elapsed time
	 * between two calls to this function.
	 */
	static double mu_timer_real(void)
	{

		return -1.0; /* Failed. */
	}

	/**
	 * Returns the amount of CPU time used by the current process,
	 * in seconds, or -1.0 if an error occurred.
	 */
	static double mu_timer_cpu(void)
	{
		return -1; /* Failed. */
	}

#ifdef __cplusplus
}
#endif

#endif /* MINUNIT_MINUNIT_H */
