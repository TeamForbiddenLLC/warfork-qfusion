#include "../qstr.h"
#include "utest.h"

UTEST(qstr, qCaselessCompare)
{
    EXPECT_EQ(qStrCaselessCompare(qCToStrRef("test"), qCToStrRef("TEST")), 0);
    EXPECT_EQ(qStrCaselessCompare(qCToStrRef("testAA"), qCToStrRef("TEST")), 1);
}

UTEST(qstr, qCompare)
{
    EXPECT_EQ(qStrCompare(qCToStrRef("test"), qCToStrRef("TEST")), 32);
    EXPECT_EQ(qStrCompare(qCToStrRef("Test"), qCToStrRef("test")), -32);
}

UTEST(qstr, qIndexOf)
{
    EXPECT_EQ(qStrIndexOf(qCToStrRef("foo foo"), qCToStrRef("foo")), 0);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("banana"), qCToStrRef("ana")), 1);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("textbook"), qCToStrRef("book")), 4);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("abababcabababcbab"), qCToStrRef("ababc")), 2);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("This is a test string to see how Boyer-Moore handles longer searches"), qCToStrRef("searches")), 60);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("hello world"), qCToStrRef("hello")), 0);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("apple banana"), qCToStrRef("nana")), 8);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("goodbye"), qCToStrRef("bye")), 4);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("cat"), qCToStrRef("dog")), -1);
    EXPECT_EQ(qStrIndexOf(qCToStrRef(""), qCToStrRef("abc")), -1);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("hello"), qCToStrRef("")), -1);
    EXPECT_EQ(qStrIndexOf(qCToStrRef("abc"), qCToStrRef("abcd")), -1);
}

UTEST(qstr, qStrReadDouble) 
{
    double result = 0;
    EXPECT_TRUE(qStrReadDouble(qCToStrRef(".01"), &result));
    EXPECT_NEAR(result, .01f, .0001f);
    
    EXPECT_TRUE(qStrReadDouble(qCToStrRef("5.01"), &result));
    EXPECT_NEAR(result, 5.01f, .0001f);
    
    EXPECT_TRUE(qStrReadDouble(qCToStrRef("5.01"), &result));
    EXPECT_NEAR(result, 5.01f, .0001f);
    
    EXPECT_TRUE(qStrReadDouble(qCToStrRef("-5.01"), &result));
    EXPECT_NEAR(result, -5.01f, .0001f);
}

UTEST(qstr, qStrReadFloat)
{
    float result = 0;
    EXPECT_TRUE(qStrReadFloat(qCToStrRef(".01"), &result));
    EXPECT_NEAR(result, .01f, .0001f);
    
    EXPECT_TRUE(qStrReadFloat(qCToStrRef("5.01"), &result));
    EXPECT_NEAR(result, 5.01f, .0001f);
    
    EXPECT_TRUE(qStrReadFloat(qCToStrRef("5.01"), &result));
    EXPECT_NEAR(result, 5.01f, .0001f);
    
    EXPECT_TRUE(qStrReadFloat(qCToStrRef("-5.01"), &result));
    EXPECT_NEAR(result, -5.01f, .0001f);

}

UTEST(qstr, qLastIndexOf)
{
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("foo foo"), qCToStrRef(" ")), 3);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("foo foo"), qCToStrRef("foo")), 4);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("banana"), qCToStrRef("ana")), 3);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("textbook"), qCToStrRef("book")), 4);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("abababcabababcbab"), qCToStrRef("ababc")), 9);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("This is a test string to see how Boyer-Moore handles longer searches"), qCToStrRef("searches")), 60);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("hello world"), qCToStrRef("hello")), 0);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("apple banana"), qCToStrRef("nana")), 8);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("goodbye"), qCToStrRef("bye")), 4);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("cat"), qCToStrRef("dog")), -1);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef(""), qCToStrRef("abc")), -1);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("hello"), qCToStrRef("")), -1);
    EXPECT_EQ(qStrLastIndexOf(qCToStrRef("abc"), qCToStrRef("abcd")), -1);
}

UTEST(qstr, qEq)
{
    EXPECT_EQ(qStrEqual(qCToStrRef("Hello world"), qCToStrRef("Hello world")), 1);
    EXPECT_EQ(qStrEqual(qCToStrRef("Helloworld"), qCToStrRef("Hello world")), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("Hello World"), qCToStrRef("Hello world")), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("Hello"), qCToStrRef("Hello ")), 0);
}

UTEST(qstr, qDuplicate)
{
    struct QStr s1 = { 0 };
    EXPECT_EQ(qStrAssign(&s1, qCToStrRef("Hello World")), true);
    struct QStr s2 = qStrDup(&s1);
    EXPECT_NE(s1.buf, s2.buf);
    qStrFree(&s1);
    qStrFree(&s2);
}

UTEST(qstr, qIterateRev)
{
    struct QStrSpan           buf = qCToStrRef("one two three four five");
    struct qStrSplitIterable iterable = { buf, qCToStrRef(" "), buf.len };
    struct QStrSpan           s = { 0 };
    s = qStrSplitRevIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("five"), s), 1);
    s = qStrSplitRevIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("four"), s), 1);
    s = qStrSplitRevIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("three"), s), 1);
    s = qStrSplitRevIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("two"), s), 1);
    s = qStrSplitRevIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("one"), s), 1);
    s = qStrSplitRevIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 1);
}

UTEST(qstr, qReadull)
{
    unsigned long long res = 0;
    EXPECT_EQ(qStrReadull(qCToStrRef("123456"), &res), 1);
    EXPECT_EQ(res, 123456);
}

UTEST(qstr, qReadu32)
{
    long long res = 0;
    EXPECT_EQ(qStrReadll(qCToStrRef("123456"), &res), 1);
    EXPECT_EQ(res, 123456);
}

UTEST(qstr, qIterateWhiteSpace)
{
    struct qStrSplitIterable iterable = { qCToStrRef("one  two"), qCToStrRef(" "), 0 };
    struct QStrSpan           s = { 0 };

    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("one"), s), 1);

    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 1);

    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEqual(qCToStrRef("two"), s), 1);

    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 1);
}

UTEST(qstr, qIterateCount)
{
    {
        struct qStrSplitIterable iterable = { qCToStrRef("one two three four five"), qCToStrRef(" "), 0 };
        struct QStrSpan slices[] = {
            qCToStrRef("one"), qCToStrRef("two"), qCToStrRef("three"), qCToStrRef("four"), qCToStrRef("five"),
        };
        size_t i = 0;
        while (iterable.cursor < iterable.buffer.len)
        {
            struct QStrSpan it = qStrSplitIter(&iterable);
            EXPECT_EQ(qStrEqual(slices[i++], it), 1);
        }
    }
    {
        struct qStrSplitIterable iterable = { qCToStrRef("one   two"), qCToStrRef(" "), 0 };
        struct QStrSpan slices[] = {
            qCToStrRef("one"),
            qCToStrRef(""),
            qCToStrRef(""),
            qCToStrRef("two"),
        };
        size_t i = 0;
        while (iterable.cursor < iterable.buffer.len)
        {
            struct QStrSpan it = qStrSplitIter(&iterable);
            EXPECT_EQ(qStrEqual(slices[i++], it), 1);
        }
    }
}

UTEST(qstr, qIterate)
{
    struct qStrSplitIterable iterable = { qCToStrRef("one two three four five"), qCToStrRef(" "),  0 };

    struct QStrSpan s = { 0 };
    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("one"), s), 1);
    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("two"), s), 1);
    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("three"), s), 1);
    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("four"), s), 1);
    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 0);
    EXPECT_EQ(qStrEqual(qCToStrRef("five"), s), 1);
    s = qStrSplitIter(&iterable);
    EXPECT_EQ(qStrEmpty(s), 1);
}

UTEST(qstr, qfmtWriteLongLong)
{
    char   test_buffer[QSTR_LLSTR_SIZE];
    size_t len = qstrfmtll((struct QStrSpan){  test_buffer, QSTR_LLSTR_SIZE }, 12481);
    EXPECT_EQ(len, 5);
    EXPECT_EQ(qStrEqual(qCToStrRef("12481"), (struct QStrSpan){ test_buffer, len }), true);
}

UTEST(qstr, qCaselessEq)
{
    EXPECT_EQ(qStrCaselessEqual(qCToStrRef("Hello world"), qCToStrRef("Hello world")), 1);
    EXPECT_EQ(qStrCaselessEqual(qCToStrRef("Helloworld"), qCToStrRef("Hello world")), 0);
    EXPECT_EQ(qStrCaselessEqual(qCToStrRef("Hello World"), qCToStrRef("Hello world")), 1);
    EXPECT_EQ(qStrCaselessEqual(qCToStrRef("Hello"), qCToStrRef("Hello ")), 0);
}

UTEST(qstr, qCatJoin)
{
    struct QStr     buf = { 0 };
    struct QStrSpan slices[] = {
        qCToStrRef("one"),
        qCToStrRef("two"),
        qCToStrRef("three"),
        qCToStrRef("four"),
    };
    EXPECT_EQ(qstrcatjoin(&buf, slices, 4, qCToStrRef(" ")), true);
    EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("one two three four")), true);
    qStrFree(&buf);
}

UTEST(qstr, qCatJoinCstr)
{
    struct QStr buf = { 0 };
    const char* slices[] = {
        "one",
        "two",
        "three",
        "four",
    };
    EXPECT_EQ(qstrcatjoinCStr(&buf, slices, 4, qCToStrRef(" ")), true);
    EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("one two three four")), true);
    qStrFree(&buf);
}

UTEST(qstr, appendSlice)
{
    struct QStr buf = { 0 };
    qStrAssign(&buf, qCToStrRef("Hello"));
    EXPECT_EQ(qStrAppendSlice(&buf, qCToStrRef(" world")), true);
    EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("Hello world")), true);
    qStrFree(&buf);
}

UTEST(qstr, qcatprintf)
{
    struct QStr buf = { 0 };
    EXPECT_EQ(qstrcatprintf(&buf, "Hello %s", "world"), true);
    EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("Hello world")), true);

    qStrSetLen(&buf, 0);
    EXPECT_EQ(qstrcatprintf(&buf, "%d", 123), true);
    EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("123")), true);

    EXPECT_EQ(qstrcatprintf(&buf, " %lu", 156), true);
    EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("123 156")), true);

    qStrSetLen(&buf, 0);
    EXPECT_EQ(qstrcatprintf(&buf, "a%cb", 0), true);
    EXPECT_EQ(qStrEqual(qToStrRef(buf), (struct QStrSpan){ (char*)"a\0" "b",(size_t)3 }), 1);
    qStrFree(&buf);
}

UTEST(qstr, qcatfmt)
{
    struct QStr s = { 0 };
    {
        qstrcatfmt(&s, "%S\n", qCToStrRef("Hello World"));
        EXPECT_EQ(qStrEqual(qToStrRef(s), qCToStrRef("Hello World\n")), true);
        qStrClear(&s);
    }
    {
        qstrcatfmt(&s, "%i\n", 125);
        EXPECT_EQ(qStrEqual(qToStrRef(s), qCToStrRef("125\n")), true);
        qStrClear(&s);
    }
    {
        qstrcatfmt(&s, "%i\n", -125);
        EXPECT_EQ(qStrEqual(qToStrRef(s), qCToStrRef("-125\n")), true);
        qStrClear(&s);
    }
    qStrFree(&s);
}

UTEST(qstr, updateLen)
{
    struct QStr buf = { 0 };
    qStrAssign(&buf, qCToStrRef("Hello World"));
    buf.buf[5] = '\0';
    qStrUpdateLen(&buf);

    EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("Hello")), true);
    EXPECT_EQ(buf.len, 5);

    qStrFree(&buf);
}

UTEST(qstr, qStrAssign)
{
    struct QStr buf = { 0 };
    {
        qStrAssign(&buf, qCToStrRef("Hello World"));
        EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("Hello World")), true);
    }
    {
        qStrAssign(&buf, qStrTrim(qCToStrRef("   Hello World   ")));
        EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("Hello World")), true);
    }
    qStrFree(&buf);
}

UTEST(qstr, q_rtrim)
{
    EXPECT_EQ(qStrEqual(qStrRTrim(qCToStrRef("Hello world  ")), qCToStrRef("Hello world")), true);
    EXPECT_EQ(qStrEqual(qStrRTrim(qCToStrRef("  Hello world  ")), qCToStrRef("  Hello world")), true);
    EXPECT_EQ(qStrEqual(qStrRTrim(qCToStrRef("  ")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrRTrim(qCToStrRef(" ")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrRTrim(qCToStrRef("\n")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrRTrim(qCToStrRef("\t")), qCToStrRef("")), true);
}

UTEST(qstr, q_ltrim)
{
    EXPECT_EQ(qStrEqual(qStrLTrim(qCToStrRef("Hello world  ")), qCToStrRef("Hello world  ")), true);
    EXPECT_EQ(qStrEqual(qStrLTrim(qCToStrRef("  ")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrLTrim(qCToStrRef(" ")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrLTrim(qCToStrRef("\n")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrLTrim(qCToStrRef("\t")), qCToStrRef("")), true);
}

UTEST(qstr, qStrTrim)
{
    {
        struct QStr buf = { 0 };
        qStrAssign(&buf, qCToStrRef("  Hello World "));
        qStrAssign(&buf, qStrTrim(qToStrRef(buf)));
        EXPECT_EQ(qStrEqual(qToStrRef(buf), qCToStrRef("Hello World")), true);
        qStrFree(&buf);
    }

    EXPECT_EQ(qStrEqual(qStrTrim(qCToStrRef("Hello world  ")), qCToStrRef("Hello world")), true);
    EXPECT_EQ(qStrEqual(qStrTrim(qCToStrRef("  \t Hello world  ")), qCToStrRef("Hello world")), true);
    EXPECT_EQ(qStrEqual(qStrTrim(qCToStrRef("  ")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrTrim(qCToStrRef(" ")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrTrim(qCToStrRef("\n")), qCToStrRef("")), true);
    EXPECT_EQ(qStrEqual(qStrTrim(qCToStrRef("\t")), qCToStrRef("")), true);
}

UTEST(qstr, qsscanf)
{
    {
        uint32_t a = 0;
        uint32_t b = 0;
        int      read = qstrsscanf(qCToStrRef("10.132"), "%u.%u", &a, &b);
        EXPECT_EQ(read, 2);
        EXPECT_EQ(a, 10);
        EXPECT_EQ(b, 132);
    }
}

// UTEST(qstr, qSliceToUtf16CodePoint) {
//   unsigned char leftPointingMagnify[] = {0x3D,0xD8,0x0D,0xDD};
//
//   struct tf_utf_result_s res;
//   {
//     struct qStrSplitIterable iter = {
//       .buffer = (struct QStrSpan) {
//         .buf = (const char*)leftPointingMagnify,
//         .len = 2,
//       },
//       .cursor = 0
//     };
//     res = tfUtf16NextCodePoint(&iter);
//     EXPECT_EQ((bool)res.invalid, true);
//     EXPECT_EQ((bool)res.finished, true);
//   }
//   {
//     struct qStrSplitIterable iter = {
//       .buffer = (struct QStrSpan) {
//         .buf = (const char*)leftPointingMagnify,
//         .len = sizeof(leftPointingMagnify),
//       },
//       .cursor = 0
//     };
//     res = tfUtf16NextCodePoint(&iter);
//     EXPECT_EQ(res.codePoint, 0x1F50D);

//     EXPECT_EQ((bool)res.finished, true);
//   }
//   {
//     struct qStrSplitIterable iter = (struct qStrSplitIterable){
//       .buffer = (struct QStrSpan) {
//         .buf = NULL,
//         .len = 0,
//       },
//       .cursor = 0
//     };
//     res = tfUtf16NextCodePoint(&iter);
//     EXPECT_EQ((bool)res.invalid, true);
//     EXPECT_EQ((bool)res.finished, true);
//   }
//   {
//     char badInput[] = {0};
//     struct qStrSplitIterable iter = (struct qStrSplitIterable){
//         .buffer = (struct QStrSpan){
//           .buf = badInput,
//           .len = 0,
//         },
//         .cursor = 0};
//     res = tfUtf16NextCodePoint(&iter);
//     EXPECT_EQ((bool)res.invalid, true);
//     EXPECT_EQ((bool)res.finished, true);
//   }
//
// }

// UTEST(qstr, qSliceToUtf8CodePoint) {
//   char smilyCat[] = {0xF0, 0x9F, 0x98, 0xBC};
//   struct qStrSplitIterable iter = {
//     .buffer = (struct QStrSpan) {
//       .len = sizeof(smilyCat),
//       .buf = smilyCat
//     },
//     .cursor = 0
//   };
//
//   struct tf_utf_result_s res = tfUtf8NextCodePoint(&iter);
//   EXPECT_EQ(res.codePoint, 0x0001f63c);
//
//   char charU[] = {'U'};
//   EXPECT_EQ(tfSliceToUtf8CodePoint((struct QStrSpan) {
//     .len = sizeof(charU),
//     .buf = charU
//   }, 0), 'U');
//
//   char ringOperator[] = {0xe2, 0x88, 0x98};
//   EXPECT_EQ(tfSliceToUtf8CodePoint((struct QStrSpan) {
//     .len = sizeof(ringOperator),
//     .buf = ringOperator
//   }, 0), 0x2218);
//
//   // this has an extra byte
//   char badRingOperator[] = {0xe2, 0x88, 0x98, 0x1};
//   EXPECT_EQ(tfSliceToUtf8CodePoint((struct QStrSpan) {
//     .len = sizeof(badRingOperator),
//     .buf = badRingOperator
//   }, 1), 1);
// }

// UTEST(qstr, qUtf8CodePointIter) {
//   {
//     unsigned char smilyCat[] = {0xF0, 0x9F, 0x98, 0xBC};
//     struct qStrSplitIterable iter = {
//       .buffer = (struct QStrSpan) {
//         .buf = (char*)smilyCat,
//         .len = sizeof(smilyCat),
//       },
//       .cursor = 0
//     };
//     struct tf_utf_result_s res = tfUtf8NextCodePoint(&iter);
//     EXPECT_EQ(res.codePoint, 0x0001f63c);
//     EXPECT_EQ((bool)res.finished, true);
//     EXPECT_EQ((bool)res.invalid, false);
//   }
// {
//   // Ḽơᶉëᶆ
//   unsigned char buffer[] = {0xE1, 0xB8, 0xBC, 0xC6, 0xA1, 0xE1, 0xB6, 0x89,0xC3, 0xAB,0xE1, 0xB6,0x86 };
//   struct qStrSplitIterable iterable = {
//     .buffer = {
//       .buf = (const char*)buffer,
//       .len = sizeof(buffer)
//     },
//     .cursor = 0,
//   };
//   struct tf_utf_result_s s = {0};
//   s = tfUtf8NextCodePoint(&iterable);
//   EXPECT_EQ(s.codePoint, 0x00001E3C); // 0xE1, 0xB8, 0xBC
//   EXPECT_EQ((bool)s.finished, false);
//   EXPECT_EQ((bool)s.invalid, false);
//   s = tfUtf8NextCodePoint(&iterable);
//   EXPECT_EQ(s.codePoint,0x1a1); // 0xBC, 0xC6
//   EXPECT_EQ((bool)s.finished, false);
//   EXPECT_EQ((bool)s.invalid, false);
//   s = tfUtf8NextCodePoint(&iterable);
//   EXPECT_EQ(s.codePoint,0x1D89); // 0xE1, 0xB6, 0x89
//   EXPECT_EQ((bool)s.finished, false);
//   EXPECT_EQ((bool)s.invalid, false);
//   s = tfUtf8NextCodePoint(&iterable);
//   EXPECT_EQ(s.codePoint,0x00EB); // 0xC3 0xAB
//   EXPECT_EQ((bool)s.finished, false);
//   EXPECT_EQ((bool)s.invalid, false);
//   s = tfUtf8NextCodePoint(&iterable);
//   EXPECT_EQ(s.codePoint,0x1D86); //0xE1 0xB6 0x86
//   EXPECT_EQ((bool)s.finished, true);
//   EXPECT_EQ((bool)s.invalid, false);
//   s = tfUtf8NextCodePoint(&iterable);
//   EXPECT_EQ((bool)s.finished, true);
//   EXPECT_EQ((bool)s.invalid, true);
//   }
// }

UTEST_MAIN();

