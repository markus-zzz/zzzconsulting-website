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

The design of MyCC borrows heavily from LLVM. One of the best things about LLVM
is the use of a very well defined intermediate representation in the middle
end. This LLVM IR is linearised and resembles assembly language so much that
they in fact call it [the LLVM Assembly
Language](https://llvm.org/docs/LangRef.html).

As stated in that document one of the design criteria for the IR was to
represents well both in graph form in compiler memory as well as in human
readable textual form in dump files. The importance of the latter cannot be
overstated.

While a well defined IR allows us to import and export the entire compilation
state between passes it also allows us to create a virtual machine that can
execute this IR. Together these two provide for the development of some very
powerful testing tool.


## Preparations
```
export ZZZ_ROOT=some/path
cd $ZZZ_ROOT
git clone https://github.com/markus-zzz/mycc.git
mkdir $ZZZ_ROOT/mycc/build
cd $ZZZ_ROOT/mycc/build
make -f $ZZZ_ROOT/mycc/src/Makefile
cd $ZZZ_ROOT/mycc/test
./runner.pl
```

## A tour down the compilation pipeline
Let us take a look at a simple example and see how the code gets transformed as
we move further down the compilation pipeline. Consider something simple such
as *dot.c* given below
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
Begin by invoking the compiler driver as in
```
./build/driver --dump-all dot.c
```
doing so should produce a set of dump files where the intermediate
representation at the given stage is dumped. These files are very important for
debugging since they often times allow you to pinpoint at what stage things go
wrong

- [ast_00_pristine.txt]({{site.url}}/download/mycc-an-introduction/ast_00_pristine.txt) -
When the front end parses the input code it produces an Abstract Syntax Tree
(AST). This representation is extremely verbose and essentially contains an AST
node for each token in the input.
- [ir_00_pristine.txt]({{site.url}}/download/mycc-an-introduction/ir_00_pristine.txt) -
Middle end Intermediate Representation (what we simply call IR) immediately
after being translated from AST.
- [ir_01_mem2reg.txt]({{site.url}}/download/mycc-an-introduction/ir_01_mem2reg.txt) -
IR after applying the *mem2reg* optimization pass.
- [cg_00_iselect.txt]({{site.url}}/download/mycc-an-introduction/cg_00_iselect.txt) -
Code Generator intermediate representation after selecting machine instructions
for the middle end IR. Note that this IR is using virtual registers and is
still in SSA form.
- [cg_01_regalloc.txt]({{site.url}}/download/mycc-an-introduction/cg_01_regalloc.txt) -
Machine instructions after allocating physical registers.
- [cg_02_branch_predication.txt]({{site.url}}/download/mycc-an-introduction/cg_02_branch_predication.txt) -
Machine instructions after performing branch predication optimization pass.

