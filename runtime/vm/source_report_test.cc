// Copyright (c) 2015, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/source_report.h"
#include "vm/dart_api_impl.h"
#include "vm/unit_test.h"

namespace dart {

#ifndef PRODUCT

static ObjectPtr ExecuteScript(const char* script, bool allow_errors = false) {
  Dart_Handle lib;
  {
    TransitionVMToNative transition(Thread::Current());
    if (allow_errors) {
      lib = TestCase::LoadTestScriptWithErrors(script, NULL);
    } else {
      lib = TestCase::LoadTestScript(script, NULL);
    }
    EXPECT_VALID(lib);
    Dart_Handle result = Dart_Invoke(lib, NewString("main"), 0, NULL);
    EXPECT_VALID(result);
  }
  return Api::UnwrapHandle(lib);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_NoCalls) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "main() {\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));
  SourceReport report(SourceReport::kCoverage);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("libraries", json_str, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":"

      // One compiled range, one hit at function declaration.
      "[{\"scriptIndex\":0,\"startPos\":0,\"endPos\":9,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0],\"misses\":[]}}],"

      // One script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_SimpleCall) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "helper1() {}\n"
      "main() {\n"
      "  if (true) {\n"
      "    helper0();\n"
      "  } else {\n"
      "    helper1();\n"
      "  }\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // One range compiled with one hit at function declaration (helper0).
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0],\"misses\":[]}},"

      // One range not compiled (helper1).
      "{\"scriptIndex\":0,\"startPos\":13,\"endPos\":24,\"compiled\":false},"

      // One range with two hits and a miss (main).
      "{\"scriptIndex\":0,\"startPos\":26,\"endPos\":94,\"compiled\":true,"
      "\"coverage\":{\"hits\":[26,53],\"misses\":[79]}}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_ForceCompile) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "helper1() {}\n"
      "main() {\n"
      "  if (true) {\n"
      "    helper0();\n"
      "  } else {\n"
      "    helper1();\n"
      "  }\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage, SourceReport::kForceCompile);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);

  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // One range compiled with one hit at function declaration (helper0).
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0],\"misses\":[]}},"

      // This range is compiled even though it wasn't called (helper1).
      "{\"scriptIndex\":0,\"startPos\":13,\"endPos\":24,\"compiled\":true,"
      "\"coverage\":{\"hits\":[],\"misses\":[13]}},"

      // One range with two hits and a miss (main).
      "{\"scriptIndex\":0,\"startPos\":26,\"endPos\":94,\"compiled\":true,"
      "\"coverage\":{\"hits\":[26,53],\"misses\":[79]}}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_UnusedClass_NoForceCompile) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "class Unused {\n"
      "  helper1() { helper0(); }\n"
      "}\n"
      "main() {\n"
      "  helper0();\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // UnusedClass is not compiled.
      "{\"scriptIndex\":0,\"startPos\":13,\"endPos\":55,\"compiled\":false},"

      // helper0 is compiled.
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0],\"misses\":[]}},"

      // One range with two hits (main).
      "{\"scriptIndex\":0,\"startPos\":57,\"endPos\":79,\"compiled\":true,"
      "\"coverage\":{\"hits\":[57,68],\"misses\":[]}}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_UnusedClass_ForceCompile) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "class Unused {\n"
      "  helper1() { helper0(); }\n"
      "}\n"
      "main() {\n"
      "  helper0();\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage, SourceReport::kForceCompile);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // UnusedClass.helper1 is compiled.
      "{\"scriptIndex\":0,\"startPos\":30,\"endPos\":53,\"compiled\":true,"
      "\"coverage\":{\"hits\":[],\"misses\":[30,42]}},"

      // helper0 is compiled.
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0],\"misses\":[]}},"

      // One range with two hits (main).
      "{\"scriptIndex\":0,\"startPos\":57,\"endPos\":79,\"compiled\":true,"
      "\"coverage\":{\"hits\":[57,68],\"misses\":[]}}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_UnusedClass_ForceCompileError) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "class Unused {\n"
      "  helper1() { helper0()+ }\n"  // syntax error
      "}\n"
      "main() {\n"
      "  helper0();\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript, true);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage, SourceReport::kForceCompile);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // UnusedClass has a syntax error.
      "{\"scriptIndex\":0,\"startPos\":30,\"endPos\":53,\"compiled\":false,"
      "\"error\":{\"type\":\"@Error\",\"_vmType\":\"LanguageError\","
      "\"kind\":\"LanguageError\",\"id\":\"objects\\/0\","
      "\"message\":\"'file:\\/\\/\\/test-lib': error: "
      "\\/test-lib:3:26: "
      "Error: This couldn't be parsed.\\n"
      "  helper1() { helper0()+ }\\n                         ^\"}},"

      // helper0 is compiled.
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0],\"misses\":[]}},"

      // One range with two hits (main).
      "{\"scriptIndex\":0,\"startPos\":57,\"endPos\":79,\"compiled\":true,"
      "\"coverage\":{\"hits\":[57,68],\"misses\":[]}}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_NestedFunctions) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {\n"
      "  nestedHelper0() {}\n"
      "  nestedHelper1() {}\n"
      "  nestedHelper0();\n"
      "}\n"
      "helper1() {}\n"
      "main() {\n"
      "  if (true) {\n"
      "    helper0();\n"
      "  } else {\n"
      "    helper1();\n"
      "  }\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);

  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // One range compiled with one hit (helper0).
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":73,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0,56],\"misses\":[]}},"

      // One range not compiled (helper1).
      "{\"scriptIndex\":0,\"startPos\":75,\"endPos\":86,\"compiled\":false},"

      // One range with two hits and a miss (main).
      "{\"scriptIndex\":0,\"startPos\":88,\"endPos\":156,\"compiled\":true,"
      "\"coverage\":{\"hits\":[88,115],\"misses\":[141]}},"

      // Nested range compiled (nestedHelper0).
      "{\"scriptIndex\":0,\"startPos\":14,\"endPos\":31,\"compiled\":true,"
      "\"coverage\":{\"hits\":[14],\"misses\":[]}},"

      // Nested range not compiled (nestedHelper1).
      "{\"scriptIndex\":0,\"startPos\":35,\"endPos\":52,\"compiled\":false}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_RestrictedRange) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {\n"
      "  nestedHelper0() {}\n"
      "  nestedHelper1() {}\n"
      "  nestedHelper0();\n"
      "}\n"
      "helper1() {}\n"
      "main() {\n"
      "  if (true) {\n"
      "    helper0();\n"
      "  } else {\n"
      "    helper1();\n"
      "  }\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));
  const Function& helper = Function::Handle(
      lib.LookupLocalFunction(String::Handle(String::New("helper0"))));

  SourceReport report(SourceReport::kCoverage);
  JSONStream js;
  // Restrict the report to only helper0 and it's nested functions.
  report.PrintJSON(&js, script, helper.token_pos(), helper.end_token_pos());
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);

  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // One range compiled with one hit (helper0).
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":73,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0,56],\"misses\":[]}},"

      // Nested range compiled (nestedHelper0).
      "{\"scriptIndex\":0,\"startPos\":14,\"endPos\":31,\"compiled\":true,"
      "\"coverage\":{\"hits\":[14],\"misses\":[]}},"

      // Nested range not compiled (nestedHelper1).
      "{\"scriptIndex\":0,\"startPos\":35,\"endPos\":52,\"compiled\":false}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_AllFunctions) {
  const char* kScript =
      "helper0() {}\n"
      "helper1() {}\n"
      "main() {\n"
      "  if (true) {\n"
      "    helper0();\n"
      "  } else {\n"
      "    helper1();\n"
      "  }\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());

  SourceReport report(SourceReport::kCoverage);
  JSONStream js;

  // We generate a report with all functions in the VM.
  Script& null_script = Script::Handle();
  report.PrintJSON(&js, null_script);
  const char* result = js.ToCString();

  // Sanity check the header.
  EXPECT_SUBSTRING("{\"type\":\"SourceReport\",\"ranges\":[", result);

  // Make sure that the main function was found.
  EXPECT_SUBSTRING(
      "\"startPos\":26,\"endPos\":94,\"compiled\":true,"
      "\"coverage\":{\"hits\":[26,53],\"misses\":[79]}",
      result);

  // More than one script is referenced in the report.
  EXPECT_SUBSTRING("\"scriptIndex\":0", result);
  EXPECT_SUBSTRING("\"scriptIndex\":1", result);
  EXPECT_SUBSTRING("\"scriptIndex\":2", result);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_AllFunctions_ForceCompile) {
  const char* kScript =
      "helper0() {}\n"
      "helper1() {}\n"
      "main() {\n"
      "  if (true) {\n"
      "    helper0();\n"
      "  } else {\n"
      "    helper1();\n"
      "  }\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());

  SourceReport report(SourceReport::kCoverage, SourceReport::kForceCompile);
  JSONStream js;

  // We generate a report with all functions in the VM.
  Script& null_script = Script::Handle();
  report.PrintJSON(&js, null_script);
  const char* result = js.ToCString();

  // Sanity check the header.
  EXPECT_SUBSTRING("{\"type\":\"SourceReport\",\"ranges\":[", result);

  // Make sure that the main function was found.
  EXPECT_SUBSTRING(
      "\"startPos\":26,\"endPos\":94,\"compiled\":true,"
      "\"coverage\":{\"hits\":[26,53],\"misses\":[79]}",
      result);

  // More than one script is referenced in the report.
  EXPECT_SUBSTRING("\"scriptIndex\":0", result);
  EXPECT_SUBSTRING("\"scriptIndex\":1", result);
  EXPECT_SUBSTRING("\"scriptIndex\":2", result);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_CallSites_SimpleCall) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 2048;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "helper1() {}\n"
      "main() {\n"
      "  helper0();\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCallSites);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // One range compiled with no callsites (helper0).
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"callSites\":[]},"

      // One range not compiled (helper1).
      "{\"scriptIndex\":0,\"startPos\":13,\"endPos\":24,\"compiled\":false},"

      // One range compiled with one callsite (main).
      "{\"scriptIndex\":0,\"startPos\":26,\"endPos\":48,\"compiled\":true,"
      "\"callSites\":["
      "{\"name\":\"helper0\",\"tokenPos\":37,\"cacheEntries\":["
      "{\"target\":{\"type\":\"@Function\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"helper0\",\"owner\":{\"type\":\"@Library\",\"fixedId\":true,"
      "\"id\":\"\",\"name\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\"},"
      "\"_kind\":\"RegularFunction\",\"static\":true,\"const\":false,"
      "\"implicit\":false,"
      "\"_intrinsic\":false,\"_native\":false,\"location\":{\"type\":"
      "\"SourceLocation\",\"script\":{\"type\":\"@Script\",\"fixedId\":true,"
      "\"id\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"},"
      "\"tokenPos\":0,\"endTokenPos\":11}},\"count\":1}]}]}],"

      // One script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_CallSites_PolymorphicCall) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 4096;
  char buffer[kBufferSize];
  const char* kScript =
      "class Common {\n"
      "  func() {}\n"
      "}\n"
      "class Uncommon {\n"
      "  func() {}\n"
      "}\n"
      "helper(arg) {\n"
      "  arg.func();\n"
      "}\n"
      "main() {\n"
      "  Common common = new Common();\n"
      "  Uncommon uncommon = new Uncommon();\n"
      "  helper(common);\n"
      "  helper(common);\n"
      "  helper(uncommon);\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));
  const Function& helper = Function::Handle(
      lib.LookupLocalFunction(String::Handle(String::New("helper"))));

  SourceReport report(SourceReport::kCallSites);
  JSONStream js;
  report.PrintJSON(&js, script, helper.token_pos(), helper.end_token_pos());
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // One range...
      "{\"scriptIndex\":0,\"startPos\":60,\"endPos\":88,\"compiled\":true,"

      // With one call site...
      "\"callSites\":[{\"name\":\"dyn:func\",\"tokenPos\":80,\"cacheEntries\":["

      // First receiver: "Common", called twice.
      "{\"receiver\":{\"type\":\"@Class\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"Common\","
      "\"location\":{\"type\":\"SourceLocation\","
      "\"script\":{\"type\":\"@Script\","
      "\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\","
      "\"_kind\":\"kernel\"},\"tokenPos\":0,\"endTokenPos\":27},"
      "\"library\":{\"type\":\"@Library\",\"fixedId\":true,"
      "\"id\":\"\",\"name\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\"}},"

      "\"target\":{\"type\":\"@Function\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"func\","
      "\"owner\":{\"type\":\"@Class\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"Common\","
      "\"location\":{\"type\":\"SourceLocation\","
      "\"script\":{\"type\":\"@Script\","
      "\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\","
      "\"_kind\":\"kernel\"},\"tokenPos\":0,\"endTokenPos\":27},"
      "\"library\":{\"type\":\"@Library\",\"fixedId\":true,"
      "\"id\":\"\",\"name\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\"}"
      "},\"_kind\":\"RegularFunction\","
      "\"static\":false,\"const\":false,\"implicit\":false,\"_intrinsic\":"
      "false,"
      "\"_native\":false,"
      "\"location\":{\"type\":\"SourceLocation\","
      "\"script\":{\"type\":\"@Script\",\"fixedId\":true,"
      "\"id\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\","
      "\"_kind\":\"kernel\"},\"tokenPos\":17,\"endTokenPos\":25}},"

      "\"count\":2},"

      // Second receiver: "Uncommon", called once.
      "{\"receiver\":{\"type\":\"@Class\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"Uncommon\","
      "\"location\":{\"type\":\"SourceLocation\","
      "\"script\":{\"type\":\"@Script\","
      "\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\","
      "\"_kind\":\"kernel\"},\"tokenPos\":29,\"endTokenPos\":58},"
      "\"library\":{\"type\":\"@Library\",\"fixedId\":true,"
      "\"id\":\"\",\"name\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\"}},"

      "\"target\":{\"type\":\"@Function\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"func\","
      "\"owner\":{\"type\":\"@Class\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"Uncommon\","
      "\"location\":{\"type\":\"SourceLocation\","
      "\"script\":{\"type\":\"@Script\","
      "\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\","
      "\"_kind\":\"kernel\"},\"tokenPos\":29,\"endTokenPos\":58},"
      "\"library\":{\"type\":\"@Library\",\"fixedId\":true,"
      "\"id\":\"\",\"name\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\"}"
      "},\"_kind\":\"RegularFunction\","
      "\"static\":false,\"const\":false,\"implicit\":false,\"_intrinsic\":"
      "false,"
      "\"_native\":false,"
      "\"location\":{\"type\":\"SourceLocation\","
      "\"script\":{\"type\":\"@Script\",\"fixedId\":true,"
      "\"id\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\","
      "\"_kind\":\"kernel\"},\"tokenPos\":48,\"endTokenPos\":56}},"

      "\"count\":1}]}]}],"

      // One script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_MultipleReports) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 2048;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "helper1() {}\n"
      "main() {\n"
      "  helper0();\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCallSites | SourceReport::kCoverage);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // One range compiled with no callsites (helper0).
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"callSites\":[],"
      "\"coverage\":{\"hits\":[0],\"misses\":[]}},"

      // One range not compiled (helper1).
      "{\"scriptIndex\":0,\"startPos\":13,\"endPos\":24,\"compiled\":false},"

      // One range compiled with one callsite (main)m
      "{\"scriptIndex\":0,\"startPos\":26,\"endPos\":48,\"compiled\":true,"
      "\"callSites\":[{\"name\":\"helper0\",\"tokenPos\":37,\"cacheEntries\":[{"
      "\"target\":{\"type\":\"@Function\",\"fixedId\":true,\"id\":\"\","
      "\"name\":\"helper0\",\"owner\":{\"type\":\"@Library\",\"fixedId\":true,"
      "\"id\":\"\",\"name\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\"},\"_"
      "kind\":\"RegularFunction\",\"static\":true,\"const\":false,\"implicit\":"
      "false,\"_"
      "intrinsic\":false,\"_native\":false,\"location\":{\"type\":"
      "\"SourceLocation\",\"script\":{\"type\":\"@Script\",\"fixedId\":true,"
      "\"id\":\"\",\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"},"
      "\"tokenPos\":0,\"endTokenPos\":11}},\"count\":1}]}],\"coverage\":{"
      "\"hits\":[26,37],\"misses\":[]}}],"

      // One script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_PossibleBreakpoints_Simple) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "helper0() {}\n"
      "helper1() {}\n"
      "main() {\n"
      "  if (true) {\n"
      "    helper0();\n"
      "  } else {\n"
      "    helper1();\n"
      "  }\n"
      "}";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kPossibleBreakpoints);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // helper0.
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":11,\"compiled\":true,"
      "\"possibleBreakpoints\":[7,11]},"

      // One range not compiled (helper1).
      "{\"scriptIndex\":0,\"startPos\":13,\"endPos\":24,\"compiled\":false},"

      // main.
      "{\"scriptIndex\":0,\"startPos\":26,\"endPos\":94,\"compiled\":true,"
      "\"possibleBreakpoints\":[30,53,79,94]}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_Issue35453_NoSuchMethod) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "class Foo {\n"
      "  void bar() {}\n"
      "}\n"
      "class Unused implements Foo {\n"
      "  dynamic noSuchMethod(_) {}\n"
      "}\n"
      "void main() {\n"
      "  Foo().bar();\n"
      "}\n";

  Library& lib = Library::Handle();
  lib ^= ExecuteScript(kScript);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage, SourceReport::kForceCompile);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // Foo is hit.
      "{\"scriptIndex\":0,\"startPos\":14,\"endPos\":26,\"compiled\":true,"
      "\"coverage\":{\"hits\":[14],\"misses\":[]}},"

      // Unused is missed.
      "{\"scriptIndex\":0,\"startPos\":62,\"endPos\":87,\"compiled\":true,"
      "\"coverage\":{\"hits\":[],\"misses\":[62]}},"

      // Main is hit.
      "{\"scriptIndex\":0,\"startPos\":91,\"endPos\":120,\"compiled\":true,"
      "\"coverage\":{\"hits\":[91,107,113],\"misses\":[]}}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

ISOLATE_UNIT_TEST_CASE(SourceReport_Coverage_Issue47017_Assert) {
  // WARNING: This MUST be big enough for the serialised JSON string.
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  const char* kScript =
      "void foo(Object? bar) {\n"
      "  assert(bar == null);\n"
      "}\n"
      "void main() {\n"
      "  foo(null);\n"
      "}\n";

  Library& lib = Library::Handle();
  const bool old_asserts = IsolateGroup::Current()->asserts();
  IsolateGroup::Current()->set_asserts(true);
  lib ^= ExecuteScript(kScript);
  IsolateGroup::Current()->set_asserts(old_asserts);
  ASSERT(!lib.IsNull());
  const Script& script =
      Script::Handle(lib.LookupScript(String::Handle(String::New("test-lib"))));

  SourceReport report(SourceReport::kCoverage, SourceReport::kForceCompile);
  JSONStream js;
  report.PrintJSON(&js, script);
  const char* json_str = js.ToCString();
  ASSERT(strlen(json_str) < kBufferSize);
  ElideJSONSubstring("classes", json_str, buffer);
  ElideJSONSubstring("libraries", buffer, buffer);
  EXPECT_STREQ(
      "{\"type\":\"SourceReport\",\"ranges\":["

      // Foo is hit, and the assert is hit.
      "{\"scriptIndex\":0,\"startPos\":0,\"endPos\":47,\"compiled\":true,"
      "\"coverage\":{\"hits\":[0,33],\"misses\":[]}},"

      // Main is hit.
      "{\"scriptIndex\":0,\"startPos\":49,\"endPos\":76,\"compiled\":true,"
      "\"coverage\":{\"hits\":[49,65],\"misses\":[]}}],"

      // Only one script in the script table.
      "\"scripts\":[{\"type\":\"@Script\",\"fixedId\":true,\"id\":\"\","
      "\"uri\":\"file:\\/\\/\\/test-lib\",\"_kind\":\"kernel\"}]}",
      buffer);
}

#endif  // !PRODUCT

}  // namespace dart
