#include <gtest/gtest.h>
#include <string>

extern std::string JsonEscape(const std::string& s);
extern std::string MakeJsonResponse(const std::string& id, const std::string& resultJson);
extern std::string MakeJsonError(const std::string& id, int code, const std::string& message);
extern std::string ExtractMethod(const std::string& json);
extern std::string ExtractId(const std::string& json);
extern std::string ExtractArg(const std::string& json, const std::string& key);

TEST(McpJsonRpc, JsonEscapeQuotes) {
    EXPECT_EQ(JsonEscape("\""), "\\\"");
}

TEST(McpJsonRpc, JsonEscapeBackslash) {
    EXPECT_EQ(JsonEscape("\\"), "\\\\");
}

TEST(McpJsonRpc, JsonEscapeNewline) {
    EXPECT_EQ(JsonEscape("\n"), "\\n");
}

TEST(McpJsonRpc, JsonEscapeTab) {
    EXPECT_EQ(JsonEscape("\t"), "\\t");
}

TEST(McpJsonRpc, JsonEscapeCarriageReturn) {
    EXPECT_EQ(JsonEscape("\r"), "\\r");
}

TEST(McpJsonRpc, JsonEscapeMultiple) {
    EXPECT_EQ(JsonEscape("a\"b\\c\nd"), "a\\\"b\\\\c\\nd");
}

TEST(McpJsonRpc, JsonEscapeNormalString) {
    EXPECT_EQ(JsonEscape("hello"), "hello");
}

TEST(McpJsonRpc, JsonEscapeEmpty) {
    EXPECT_EQ(JsonEscape(""), "");
}

TEST(McpJsonRpc, MakeJsonResponse) {
    std::string resp = MakeJsonResponse("1", "\"ok\"");
    EXPECT_NE(resp.find("\"id\":1"), std::string::npos);
    EXPECT_NE(resp.find("\"result\":\"ok\""), std::string::npos);
}

TEST(McpJsonRpc, MakeJsonError) {
    std::string err = MakeJsonError("1", -32601, "Method not found");
    EXPECT_NE(err.find("\"code\":-32601"), std::string::npos);
    EXPECT_NE(err.find("\"message\":\"Method not found\""), std::string::npos);
}

TEST(McpJsonRpc, MakeJsonErrorWithSpecialChars) {
    std::string err = MakeJsonError("\"str-id\"", -32000, "error \"with\" quotes");
    EXPECT_NE(err.find("str-id"), std::string::npos);
    EXPECT_NE(err.find("error \\\"with\\\" quotes"), std::string::npos);
}

TEST(McpJsonRpc, ExtractMethodNormal) {
    EXPECT_EQ(ExtractMethod("{\"method\":\"initialize\"}"), "initialize");
}

TEST(McpJsonRpc, ExtractMethodMissing) {
    EXPECT_EQ(ExtractMethod("{}"), "");
}

TEST(McpJsonRpc, ExtractMethodMalformed) {
    EXPECT_EQ(ExtractMethod("{\"method\":}"), "");
}

TEST(McpJsonRpc, ExtractIdNumeric) {
    EXPECT_EQ(ExtractId("{\"id\":42}"), "42");
}

TEST(McpJsonRpc, ExtractIdString) {
    EXPECT_EQ(ExtractId("{\"id\":\"abc\"}"), "\"abc\"");
}

TEST(McpJsonRpc, ExtractIdMissing) {
    EXPECT_EQ(ExtractId("{}"), "null");
}

TEST(McpJsonRpc, ExtractIdBool) {
    EXPECT_EQ(ExtractId("{\"id\":true}"), "true");
}

TEST(McpJsonRpc, ExtractArgSimple) {
    EXPECT_EQ(ExtractArg("{\"name\":\"test\"}", "name"), "test");
}

TEST(McpJsonRpc, ExtractArgMissing) {
    EXPECT_EQ(ExtractArg("{}", "name"), "");
}

TEST(McpJsonRpc, ExtractArgNumeric) {
    EXPECT_EQ(ExtractArg("{\"count\":42}", "count"), "42");
}

TEST(McpJsonRpc, ExtractArgEscapedNewline) {
    EXPECT_EQ(ExtractArg("{\"msg\":\"line1\\nline2\"}", "msg"), "line1\nline2");
}

TEST(McpJsonRpc, ExtractArgEscapedTab) {
    EXPECT_EQ(ExtractArg("{\"msg\":\"a\\tb\"}", "msg"), "a\tb");
}

TEST(McpJsonRpc, ExtractArgEscapedCarriageReturn) {
    EXPECT_EQ(ExtractArg("{\"msg\":\"a\\rb\"}", "msg"), "a\rb");
}

TEST(McpJsonRpc, ExtractArgEscapedQuote) {
    EXPECT_EQ(ExtractArg(R"({"msg":"say \"hi\""})", "msg"), "say \"hi\"");
}

TEST(McpJsonRpc, ExtractArgEscapedBackslash) {
    EXPECT_EQ(ExtractArg("{\"msg\":\"a\\\\b\"}", "msg"), "a\\b");
}

TEST(McpJsonRpc, ExtractArgUnknownEscape) {
    EXPECT_EQ(ExtractArg("{\"msg\":\"a\\xb\"}", "msg"), "a\\xb");
}

TEST(McpJsonRpc, ExtractArgTrailingSpaces) {
    EXPECT_EQ(ExtractArg("{\"val\"  :  \"abc\"  }", "val"), "abc");
}
