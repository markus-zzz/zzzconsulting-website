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
possible purpose and depending on what your intended use case is they may in
fact make things a lot more complicated than they need to be.

## MyCC
So how much prototype is it? Well a lot!

The design borrows a lot of ideas from LLVM

- IR very similar
- mem2reg pass
- importable and exportable IR

Interesting ideas
- Simulate middle end IR after each pass
-
