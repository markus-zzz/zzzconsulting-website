---
layout: post
title:  "A LLVM exercise [draft]"
date:   2018-07-09 22:00:50 +0200
categories: compiler llvm
---

Let's write a LLVM optimization pass to familiarize ourselves with the
environment. Of course LLVM already does everything remotely interesting so in
order to have some non-trivial (or non-stupid if you will) example we are going
to replace an existing pass. The pass of choice is going to be *mem2reg* mainly
because it is quite central and I happen to have experience implementing this
very pass in a different compiler framework.

## Background
One interesting thing about LLVM is the approach to entering SSA-form (i.e.
satisfying the static single assignment condition and placement of phi-nodes).
Traditionally this is considered the front end responsibility to place phi-nodes
but LLVM however takes a different approach. Namely have the front end generate
loads and stores for each variable access. Note that technically speaking this
code is already in SSA form (it is just that it is terribly inefficient). Later
on the pass mem2reg comes along and optimizes this effectively removing almost
all memory accesses for these variables.

This is nice because it makes the front end simple, it can focus on its AST to
IR translation without additional complexities. On the downside you could argue
that it makes the compilation process inefficient because you have the front end
generate a bunch of IR that will almost immediately be optimized away in the
middle end (and I have witnessed some horrible cases of this). In my opinion
though that is the name of the game in compiler engineering, to not worrying
about everything at once but instead have each part do one thing well and have
an efficient IR and infrastructure that allows for cheap IR updates.

Let us start by looking at the LLVM IR before and after mem2reg for the following example

```c
int sum(int *data, int len)
{
  int s = 0;
  for (int i = 0; i < len; ++i) {
    s += data[i];
  }
  return s;
}
```

```
clang -target armv7h -mfloat-abi=hard sum.c -O0 -S -emit-llvm
```

```
define arm_aapcs_vfpcc i32 @sum(i32* %data, i32 %len) #0 {
entry:
  %data.addr = alloca i32*, align 4
  %len.addr = alloca i32, align 4
  %s = alloca i32, align 4
  %i = alloca i32, align 4
  store i32* %data, i32** %data.addr, align 4
  store i32 %len, i32* %len.addr, align 4
  store i32 0, i32* %s, align 4
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, i32* %i, align 4
  %1 = load i32, i32* %len.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i32*, i32** %data.addr, align 4
  %3 = load i32, i32* %i, align 4
  %arrayidx = getelementptr inbounds i32, i32* %2, i32 %3
  %4 = load i32, i32* %arrayidx, align 4
  %5 = load i32, i32* %s, align 4
  %add = add nsw i32 %5, %4
  store i32 %add, i32* %s, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %6 = load i32, i32* %i, align 4
  %inc = add nsw i32 %6, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %7 = load i32, i32* %s, align 4
  ret i32 %7
}
```

```
opt -passes mem2reg -S sum.ll -o sum-mem2reg.ll
```

```
define arm_aapcs_vfpcc i32 @sum(i32* %data, i32 %len) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i32 [ 0, %entry ], [ %add, %for.inc ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, %len
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %data, i32 %i.0
  %0 = load i32, i32* %arrayidx, align 4
  %add = add nsw i32 %s.0, %0
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret i32 %s.0
}
```
As can be seen the effect of the mem2reg pass is rather dramatic. Gone are the
useless variable loads and stores and alloca instructions.

Now let's implement our own mem2reg pass (and call it ourmem2reg) and see if we
can achieve similar results. The algoritm we will use will be the classical one
by Cytron et al. and the interested reader can find all the details in [1].

Implemented in [OurMem2Reg.cpp]({{site.url}}/download/OurMem2Reg.cpp) will describe in
details shortly.

[1] Ron Cytron, Jeanne Ferrante, Barry K. Rosen, Mark N. Wegman, and F. Kenneth
Zadeck. 1991. Efficiently computing static single assignment form and the
control dependence graph. ACM Trans. Program. Lang. Syst. 13, 4 (October 1991),
451-490. DOI=http://dx.doi.org/10.1145/115372.115320

