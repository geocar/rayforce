/*
 *   Copyright (c) 2024 Anton Kundenko <singaraiona@gmail.com>
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#define TEST_ASSERT_EQ(lhs, rhs)                                                            \
    {                                                                                       \
        obj_p le = eval_str(lhs);                                                           \
        obj_p re = eval_str(rhs);                                                           \
        str_p lns = obj_fmt(le);                                                            \
        str_p rns = obj_fmt(re);                                                            \
        TEST_ASSERT(strcmp(lns, rns) == 0, str_fmt(0, "Expected %s, got %s\n -- at: %s:%d", \
                                                   rns, lns, __FILE__, __LINE__));          \
        drop_obj(le);                                                                       \
        drop_obj(re);                                                                       \
        heap_free(lns);                                                                     \
        heap_free(rns);                                                                     \
    }

test_result_t test_lang_basic()
{
    TEST_ASSERT_EQ("(+ 1 2)", "3");
    TEST_ASSERT_EQ("(+ 1 2.2)", "3.20");
    TEST_ASSERT_EQ("(+ 1.1 2)", "3.10");
    TEST_ASSERT_EQ("(+ 1.1 2.2)", "3.30");
    TEST_ASSERT_EQ("(+ [1 2] 3)", "[4 5]");
    TEST_ASSERT_EQ("(+ 1 [2 3])", "[3 4]");
    TEST_ASSERT_EQ("(+ [1 2] [3 4])", "[4 6]");
    TEST_ASSERT_EQ("(+ [1.1 2.2] [3 4])", "[4.10 6.20]");
    TEST_ASSERT_EQ("(- 1 2)", "-1");
    TEST_ASSERT_EQ("(- [1 2] 3)", "[-2 -1]");
    TEST_ASSERT_EQ("(- 1 [2 3])", "[-1 -2]");
    TEST_ASSERT_EQ("(- [1 2] [3 4])", "[-2 -2]");
    TEST_ASSERT_EQ("(* 2 3)", "6");
    TEST_ASSERT_EQ("(* [2 3] 4)", "[8 12]");
    TEST_ASSERT_EQ("(* 2 [3 4])", "[6 8]");
    TEST_ASSERT_EQ("(* [2 3] [4 5])", "[8 15]");
    TEST_ASSERT_EQ("(/ 6 3)", "2");
    TEST_ASSERT_EQ("(/ [6 8] 2)", "[3 4]");
    TEST_ASSERT_EQ("(/ 6 [3 4])", "[2 1]");
    TEST_ASSERT_EQ("(/ [6 8] [2 4])", "[3 2]");
    TEST_ASSERT_EQ("(% 6 4)", "2");
    TEST_ASSERT_EQ("(% [6 8] 3)", "[0 2]");
    TEST_ASSERT_EQ("(% 6 [4 3])", "[2 0]");
    TEST_ASSERT_EQ("(% [6 8] [4 3])", "[2 2]");
    TEST_ASSERT_EQ("(div 6 3)", "2.00");
    TEST_ASSERT_EQ("(div [6 8] 4)", "[1.50 2.00]");
    TEST_ASSERT_EQ("(div 6 [3 4])", "[2.00 1.50]");
    TEST_ASSERT_EQ("(div [6 8] [4 3])", "[1.50 2.67]");

    PASS();
}

test_result_t test_lang_query()
{
    TEST_ASSERT_EQ("(set t (table [sym price volume tape] (list [apl vod god] [102 99 203] [500 400 900] (list \"A\"\"B\"\"C\"))))",
                   "(table [sym price volume tape] (list [apl vod god] [102 99 203] [500 400 900] (list \"A\"\"B\"\"C\")))");
    TEST_ASSERT_EQ("(at t 'sym)", "[apl vod god]");
    TEST_ASSERT_EQ("(at t 'price)", "[102 99 203]");
    TEST_ASSERT_EQ("(at t 'volume)", "[500 400 900]");
    TEST_ASSERT_EQ("(at t 'tape)", "(list \"A\"\"B\"\"C\")");
    TEST_ASSERT_EQ("(set n 10)"
                   "(set ids (take n (guid 3)))"
                   "(set t (table [OrderId Symbol Price Size Tape Timestamp]"
                   "(list ids"
                   "(take n [apll good msfk ibmd amznt fbad baba])"
                   "(as 'F64 (til n))"
                   "(take n (+ 1 (til 3)))"
                   "(map (fn [x] (as 'String x)) (take n (til 10)))"
                   "(as 'Timestamp (til n)))))"
                   "null",
                   "null");
    TEST_ASSERT_EQ("(select {from: t by: Symbol})",
                   "(table [Symbol OrderId Price Size Tape Timestamp]"
                   "(list [apll good msfk ibmd amznt fbad baba]"
                   "(at ids (til 7)) [0 1 2 3 4 5 6.0] [1 2 3 1 2 3 1]"
                   "(list \"0\"\"1\"\"2\"\"3\"\"4\"\"5\"\"6\")"
                   "(at (at t 'Timestamp) (til 7))))");
    TEST_ASSERT_EQ("(select {from: t by: Symbol where: (== Price 3)})",
                   "(table [Symbol OrderId Price Size Tape Timestamp]"
                   "(list [ibmd] (at ids 3) [3.00] [1] (list \"3\") [2000.01.01D00:00:00.000000003]))");
    TEST_ASSERT_EQ("(select {from: t by: Symbol where: (== Price 99)})",
                   "(table [Symbol OrderId Price Size Tape Timestamp]"
                   "(list [] [] [] [] (list) []))");
    PASS();
}
