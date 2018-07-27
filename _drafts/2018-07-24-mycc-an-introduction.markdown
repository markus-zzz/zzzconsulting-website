---
layout: post
title:  "MyCC - an introduction"
comments: true
categories: compiler mycc
---
Today we will be looking at a prototype C compiler that I have been working on
every now and then for a while. Of course it is nowhere near complete nor even
useful for any particular purpose at all besides perhaps my own amusement and
possibly that of others.

Still I think that it serves as an interesting exercise to start from a clean
slate every now and then and do things the way that you feel are right and not
being burdened by any particular framework.

Say for instance that you read this really interesting article about
implementing a SSA based register allocator and now you want to try it out. So
you turn to a well established compiler framework like LLVM or GCC. Okay, so
the code base is huge and the compiler build time is insane and you just wanted
to play around a bit.

Now I do not want to bash on big compiler frameworks (looking your way LLVM)
but the fact remains that these huge frameworks are not suitable for every
possible purpose and depending on what your intended purpose is they may in
fact make things a lot more complicated than they need to be.

## MyCC
So how much prototype is it? Well a lot!

The design borrows a lot of ideas from LLVM

- IR very similar
- mem2reg pass
- importable and exportable IR

Interesting ideas
- Simulate middle end IR after each pass
- Simulate middle end IR after each pass

## An example
Consider something simple such as the computation of a dot like product i.e.
```
int dot(int *a, int *b, int len)
{
	int i;
	int sum = 0;
	for (i = 0; i < len; i = i + 1) {
		sum = sum + a[i] * b[i];
	}

	return sum;
}
```
Dumping of the AST nodes gives us
```
TU [0x18f8520]
\-FUNCTION_DEF [0x18f84d0]
  +-DECLARATION_SPECIFIERS [0x18f6960]
  | \-TYPE_SPECIFIER [0x18f68f0]
  |   \-INT [0x18f68a0]
  +-DECLARATOR [0x18f7280]
  | \-DIRECT_DECLARATOR [0x18f7230]
  |   +-DIRECT_DECLARATOR [0x18f6a00]
  |   | \-IDENTIFIER (dot)[0x18f69b0]
  |   \-PARAMETER_TYPE_LIST [0x18f71e0]
  |     +-PARAMETER_DECLARATION [0x18f6ca0]
  |     | +-DECLARATION_SPECIFIERS [0x18f6af0]
  |     | | \-TYPE_SPECIFIER [0x18f6aa0]
  |     | |   \-INT [0x18f6a50]
  |     | \-DECLARATOR [0x18f6c50]
  |     |   +-POINTER [0x18f6b60]
  |     |   \-DIRECT_DECLARATOR [0x18f6c00]
  |     |     \-IDENTIFIER (a)[0x18f6bb0]
  |     +-PARAMETER_DECLARATION [0x18f6f40]
  |     | +-DECLARATION_SPECIFIERS [0x18f6d90]
  |     | | \-TYPE_SPECIFIER [0x18f6d40]
  |     | |   \-INT [0x18f6cf0]
  |     | \-DECLARATOR [0x18f6ef0]
  |     |   +-POINTER [0x18f6e00]
  |     |   \-DIRECT_DECLARATOR [0x18f6ea0]
  |     |     \-IDENTIFIER (b)[0x18f6e50]
  |     \-PARAMETER_DECLARATION [0x18f7190]
  |       +-DECLARATION_SPECIFIERS [0x18f7050]
  |       | \-TYPE_SPECIFIER [0x18f6fe0]
  |       |   \-INT [0x18f6f90]
  |       \-DECLARATOR [0x18f7140]
  |         \-DIRECT_DECLARATOR [0x18f70f0]
  |           \-IDENTIFIER (len)[0x18f70a0]
  \-COMPOUND_STATEMENT [0x18f8480]
    +-DECLARATION_LIST [0x18f83e0]
    | +-DECLARATION [0x18f7570]
    | | +-DECLARATION_SPECIFIERS [0x18f7390]
    | | | \-TYPE_SPECIFIER [0x18f7320]
    | | |   \-INT [0x18f72d0]
    | | \-INIT_DECLARATOR_LIST [0x18f7520]
    | |   \-INIT_DECLARATOR [0x18f74d0]
    | |     \-DECLARATOR [0x18f7480]
    | |       \-DIRECT_DECLARATOR [0x18f7430]
    | |         \-IDENTIFIER (i)[0x18f73e0]
    | \-DECLARATION [0x18f78b0]
    |   +-DECLARATION_SPECIFIERS [0x18f7680]
    |   | \-TYPE_SPECIFIER [0x18f7610]
    |   |   \-INT [0x18f75c0]
    |   \-INIT_DECLARATOR_LIST [0x18f7860]
    |     \-INIT_DECLARATOR [0x18f7810]
    |       +-DECLARATOR [0x18f7770]
    |       | \-DIRECT_DECLARATOR [0x18f7720]
    |       |   \-IDENTIFIER (sum)[0x18f76d0]
    |       \-CONSTANT (0x0)[0x18f77c0]
    \-STATEMENT_LIST [0x18f8430]
      +-STMT_FOR [0x18f82d0]
      | +-EXPRESSION_STATEMENT [0x18f7a10]
      | | \-ASSIGN [0x18f79c0]
      | |   +-IDENTIFIER (i)[0x18f7920]
      | |   \-CONSTANT (0x0)[0x18f7970]
      | +-EXPRESSION_STATEMENT [0x18f7b90]
      | | \-LT [0x18f7b40]
      | |   +-IDENTIFIER (i)[0x18f7a80]
      | |   \-IDENTIFIER (len)[0x18f7af0]
      | +-ASSIGN [0x18f7d60]
      | | +-IDENTIFIER (i)[0x18f7c00]
      | | \-ADD [0x18f7d10]
      | |   +-IDENTIFIER (i)[0x18f7c70]
      | |   \-CONSTANT (0x1)[0x18f7cc0]
      | \-COMPOUND_STATEMENT [0x18f8280]
      |   \-STATEMENT_LIST [0x18f8230]
      |     \-EXPRESSION_STATEMENT [0x18f81e0]
      |       \-ASSIGN [0x18f8190]
      |         +-IDENTIFIER (sum)[0x18f7dd0]
      |         \-ADD [0x18f8140]
      |           +-IDENTIFIER (sum)[0x18f7e40]
      |           \-MUL [0x18f80f0]
      |             +-INDEX [0x18f7f70]
      |             | +-IDENTIFIER (a)[0x18f7eb0]
      |             | \-IDENTIFIER (i)[0x18f7f20]
      |             \-INDEX [0x18f80a0]
      |               +-IDENTIFIER (b)[0x18f7fe0]
      |               \-IDENTIFIER (i)[0x18f8050]
      \-STMT_RETURN [0x18f8390]
        \-IDENTIFIER (sum)[0x18f8340]
```
After being translated to middle end IR
```
define i32 @dot(p32, p32, i32) {
 bb1:
  %3 = alloca p32 4, 1
  %4 = getparam p32 0
  %5 = store p32 %3, %4
  %6 = alloca p32 4, 1
  %7 = getparam p32 1
  %8 = store p32 %6, %7
  %9 = alloca p32 4, 1
  %10 = getparam i32 2
  %11 = store i32 %9, %10
  %12 = alloca p32 4, 1
  %14 = alloca p32 4, 1
  %15 = alloca p32 4, 1
  %16 = const i32 0x00000000
  %17 = store i32 %15, %16
  %22 = const i32 0x00000000
  %23 = store i32 %14, %22
  br label %bb3

 bb3:
  %24 = load i32 %14
  %25 = load i32 %9
  %26 = icmp_slt i32 %24, %25
  br %26, label %bb4, label %bb6

 bb6:
  %47 = load i32 %15
  %48 = store i32 %12, %47
  br label %bb2

 bb2:
  %13 = load i32 %12
  ret i32 %13

 bb4:
  %27 = load i32 %15
  %28 = load p32 %3
  %29 = load i32 %14
  %30 = const i32 0x00000004
  %31 = mul i32 %29, %30
  %32 = add p32 %28, %31
  %33 = load i32 %32
  %34 = load p32 %6
  %35 = load i32 %14
  %36 = const i32 0x00000004
  %37 = mul i32 %35, %36
  %38 = add p32 %34, %37
  %39 = load i32 %38
  %40 = mul i32 %33, %39
  %41 = add i32 %27, %40
  %42 = store i32 %15, %41
  br label %bb5

 bb5:
  %43 = load i32 %14
  %44 = const i32 0x00000001
  %45 = add i32 %43, %44
  %46 = store i32 %14, %45
  br label %bb3

}
```
and letting mem2reg do its job
```
define i32 @dot(p32, p32, i32) {
 bb1:
  %4 = getparam p32 0
  %7 = getparam p32 1
  %10 = getparam i32 2
  %16 = const i32 0x00000000
  %22 = const i32 0x00000000
  br label %bb3

 bb3:
  %49 = phi i32 [%16, %bb1], [%41, %bb5]
  %50 = phi i32 [%22, %bb1], [%45, %bb5]
  %26 = icmp_slt i32 %50, %10
  br %26, label %bb4, label %bb6

 bb6:
  br label %bb2

 bb2:
  ret i32 %49

 bb4:
  %30 = const i32 0x00000004
  %31 = mul i32 %50, %30
  %32 = add p32 %4, %31
  %33 = load i32 %32
  %36 = const i32 0x00000004
  %37 = mul i32 %50, %36
  %38 = add p32 %7, %37
  %39 = load i32 %38
  %40 = mul i32 %33, %39
  %41 = add i32 %49, %40
  br label %bb5

 bb5:
  %44 = const i32 0x00000001
  %45 = add i32 %50, %44
  br label %bb3

}
```
