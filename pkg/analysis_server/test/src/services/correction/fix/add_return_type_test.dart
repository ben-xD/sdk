// Copyright (c) 2019, the Dart project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'package:analysis_server/src/services/correction/fix.dart';
import 'package:analysis_server/src/services/linter/lint_names.dart';
import 'package:analyzer_plugin/utilities/fixes/fixes.dart';
import 'package:test_reflective_loader/test_reflective_loader.dart';

import 'fix_processor.dart';

void main() {
  defineReflectiveSuite(() {
    defineReflectiveTests(AddReturnTypeLintTest);
    defineReflectiveTests(AddReturnTypeBulkTest);
  });
}

@reflectiveTest
class AddReturnTypeBulkTest extends BulkFixProcessorTest {
  @override
  String get lintCode => LintNames.always_declare_return_types;

  Future<void> test_singleFile() async {
    await resolveTestCode('''
class A {
  get foo => 0;
  m(p) {
    return p;
  }
}
''');
    await assertHasFix('''
class A {
  int get foo => 0;
  dynamic m(p) {
    return p;
  }
}
''');
  }
}

@reflectiveTest
class AddReturnTypeLintTest extends FixProcessorLintTest {
  @override
  FixKind get kind => DartFixKind.ADD_RETURN_TYPE;

  @override
  String get lintCode => LintNames.always_declare_return_types;

  Future<void> test_localFunction_block() async {
    await resolveTestCode('''
class A {
  void m() {
    f() {
      return '';
    }
    f();
  }
}
''');
    await assertHasFix('''
class A {
  void m() {
    String f() {
      return '';
    }
    f();
  }
}
''');
  }

  Future<void> test_localFunction_expression() async {
    await resolveTestCode('''
class A {
  void m() {
    f() => '';
    f();
  }
}
''');
    await assertHasFix('''
class A {
  void m() {
    String f() => '';
    f();
  }
}
''');
  }

  Future<void> test_method_block_noReturn() async {
    await resolveTestCode('''
class A {
  m() {
  }
}
''');
    await assertNoFix();
  }

  Future<void> test_method_block_returnDynamic() async {
    await resolveTestCode('''
class A {
  m(p) {
    return p;
  }
}
''');
    await assertHasFix('''
class A {
  dynamic m(p) {
    return p;
  }
}
''');
  }

  Future<void> test_method_block_returnNoValue() async {
    await resolveTestCode('''
class A {
  m() {
    return;
  }
}
''');
    await assertHasFix('''
class A {
  void m() {
    return;
  }
}
''');
  }

  Future<void> test_method_block_singleReturn() async {
    await resolveTestCode('''
class A {
  m() {
    return '';
  }
}
''');
    await assertHasFix('''
class A {
  String m() {
    return '';
  }
}
''');
  }

  Future<void> test_method_expression() async {
    await resolveTestCode('''
class A {
  m() => '';
}
''');
    await assertHasFix('''
class A {
  String m() => '';
}
''');
  }

  Future<void> test_method_getter() async {
    await resolveTestCode('''
class A {
  get foo => 0;
}
''');
    await assertHasFix('''
class A {
  int get foo => 0;
}
''');
  }

  Future<void> test_privateType() async {
    addSource('/home/test/lib/a.dart', '''
class A {
  _B b => _B();
}
class _B {}
''');

    await resolveTestCode('''
import 'package:test/a.dart';

f(A a) => a.b();
''');
    await assertNoFix();
  }

  Future<void> test_topLevelFunction_block() async {
    await resolveTestCode('''
f() {
  return '';
}
''');
    await assertHasFix('''
String f() {
  return '';
}
''');
  }

  Future<void> test_topLevelFunction_expression() async {
    await resolveTestCode('''
f() => '';
''');
    await assertHasFix('''
String f() => '';
''');
  }

  Future<void> test_topLevelFunction_getter() async {
    await resolveTestCode('''
get foo => 0;
''');
    await assertHasFix('''
int get foo => 0;
''');
  }
}
