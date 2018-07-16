---
layout: post
title:  "An LLVM exercise"
comments: true
categories: compiler llvm
---

Let's write an LLVM optimization pass to familiarize ourselves with the
environment. Of course LLVM already does everything even remotely interesting
so in order to have some non-trivial example (or non-stupid if you will) we are
going to replace an existing pass. The pass of choice is going to be *mem2reg*
mainly because it is quite central and I happen to have experience implementing
this very pass in a different compiler framework.

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

## Prerequisites
To follow along you will need to have an LLVM development environment setup.
This is all pretty well described
[here](https://llvm.org/docs/GettingStarted.html) but in short this is what I
did
```
export ROOT=<some path>
export PATH=$ROOT/install/bin:$PATH
mkdir -p $ROOT
mkdir -p $ROOT/build
mkdir -p $ROOT/install
cd $ROOT
wget http://releases.llvm.org/6.0.1/llvm-6.0.1.src.tar.xz
wget http://releases.llvm.org/6.0.1/cfe-6.0.1.src.tar.xz
tar xf llvm-6.0.1.src.tar.xz
cd $ROOT/llvm-6.0.1.src/tools/
tar xf $ROOT/cfe-6.0.1.src.tar.xz
cd $ROOT/build
cmake -G Ninja $ROOT/llvm-6.0.1.src/ -DCMAKE_INSTALL_PREFIX=$ROOT/install -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_LINK_LLVM_DYLIB=ON -DLLVM_TARGETS_TO_BUILD="ARM"
ninja install
```

## Experiment
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
Invoking the front end and emitting IR without applying any optimizations is as simple as
```
clang -target armv7h -mfloat-abi=hard sum.c -O0 -S -emit-llvm -o sum.ll
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
Then running the standard mem2reg pass on this IR
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
As can be seen the effects of the mem2reg pass are rather dramatic. Gone are the
inefficient variable *loads*, *stores* and *alloca* instructions.

## Implement our own
Now let's implement our own mem2reg pass (and call it ourmem2reg) and see if we
can achieve similar results. The algoritm we will use will be the classical one
by Cytron et al. and the interested reader can find all the details in [1].

The basics of writing an LLVM pass are described
[here](http://llvm.org/docs/WritingAnLLVMPass.html). Pay special attention to
the section about setting up the build environment to build the pass as a
loadable module. Doing so will save a lot of pain.

I went ahead and wrote [OurMem2Reg.cpp]({{site.url}}/download/OurMem2Reg.cpp)
and will now try to describe its contents in some detail.

### linkDefsAndUsesToVar
Given a variable (i.e. an alloca) we need to find the definitions and uses of
this variable. This corresponds to finding the store and load instructions that
use the alloca as an address. We also need to make sure that these are the only
uses of the alloca i.e. the address does not escape and allow for potential
modifictaion else where.

### renameRecursive
Here we walk the dominator tree and perform a pre-order action and a post-order action.
#### Pre-order action
Scan for def and use instructions in the block and maintain a definition stack
for each variable. A use (i.e. a load instruction) get replaced by the value on
top of the definition stack and a def (store instruction) pushes the value
stored onto the stack.
#### Post-order action
For the post-order action we need to pop the definitions put on the stack once
we are done at a level in the dominator tree and about to move up. Obviously
once we are done with a block and all the blocks that are dominated by it we
need to remove the variable definitions that came from that block since they
are no longer valid.

### runOnFunction
The driver of the pass

1. Look for *alloca* instructions (and according to LLVM conventions they are
   all located in the entry block of the function) and call
*linkDefsAndUsesToVar* on them to gather information.

1. In the previous step we took note of in which blocks there were definitions
   of the variable. Now we need to insert phi-nodes in the dominance frontier
of these blocks (and keeping in mind that when we a insert a phi-node that acts
as a new definition so we need to insert a phi-node in the dominance frontier
of that block as well). In other words we need to insert phi-nodes in the
iterated dominance frontier of the original definition blocks.  Luckily LLVM
has a new fancy framework for computing the iterated dominance frontier set
from a set of definition blocks so we simply use that (if we were to compute it
ourselves I would recommend the method described in [2]).

1. Now we enter the actual work horse of the algorithm - the renaming phase.

1. The only thing that is left to do now is to remove the trash left behind,
   i.e. removing now useless load, store and alloca instructions.

### getAnalysisUsage
Simply informing the pass manager that we need a valid dominator tree and that
we are not going to make changes to the CFG.

## Evaluate
Running the optimizer with our pass loaded as a module
```
opt -load $ROOT/build/lib/LLVMOurMem2Reg.so -ourmem2reg -S sum.ll -o sum-ourmem2reg.ll
```
giving us the following result which looks rather identical to what we get with
the stock mem2reg pass
```
define arm_aapcs_vfpcc i32 @sum(i32* %data, i32 %len) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %1 = phi i32 [ 0, %entry ], [ %add, %for.inc ]
  %cmp = icmp slt i32 %0, %len
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds i32, i32* %data, i32 %0
  %2 = load i32, i32* %arrayidx, align 4
  %add = add nsw i32 %1, %2
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret i32 %1
}
```

## Wrap up
So what have we touched in this exercise?
- Traversing the CFG
- Traversing the SSA graph (looking for uses of an instruction)
- Requesting a dominator tree and traversing it
- Using the iterated dominance frontier machinery
- Replacing instructions

Finally I make no claim that the pass implemented is 100% correct and there are
surely corner cases that would need to be handled for a proper implementation.
That however was not the purpose of this post but rather to provide an exercise
to familiarize oneself with the LLVM API using a somewhat real example doing
something useful.

In fact there is one issue if a variable is used before it is defined (e.g. not
initializing the summation variable in the example above) will result in a
compiler crash. Of course using a variable before it is defined is hardly good
practice but the pass should still handle it correctly. Fixing this is left as
an exercise for the reader but it is simply a matter of inserting a undef
instruction if we encounter a use while the definition stack is empty.

Did you like this post? Questions or feedback - leave a comment below!

## References

[1] Ron Cytron, Jeanne Ferrante, Barry K. Rosen, Mark N. Wegman, and F. Kenneth
Zadeck. 1991. Efficiently computing static single assignment form and the
control dependence graph. ACM Trans. Program. Lang. Syst. 13, 4 (October 1991),
451-490. DOI=http://dx.doi.org/10.1145/115372.115320

[2] Cooper, Keith & Harvey, Timothy & Kennedy, Ken. (2006). A Simple, Fast
Dominance Algorithm. Rice University, CS Technical Report 06-33870.

