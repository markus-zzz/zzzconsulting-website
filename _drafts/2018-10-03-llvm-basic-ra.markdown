---
layout: post
title:  "A very basic register allocator for LLVM"
math: true
comments: true
categories: compiler llvm
---

Let's do another LLVM exercise - and this time we look at implementing a very
basic register allocator inside the LLVM CodeGen framework. Again, the focus is
not to achieve an industrial strength implementation but rather to play around
and get a feel for the different APIs and IRs available inside the CodeGen.

## Background

### Liveness

* The CFG is linearized (probably RPO) and each point in the program is
  assigned a number called the **SlotIndex**. The index is incremented by 4 for
  each instruction to allow for insertion of a few new ones without having to
  renumber immediately.
* A **Segment** is an interval of the form $$[a,b)$$ where $$a$$ and $$b$$ are
  **SlotIndex**es.
* A **LiveRange** contains an ordered list of **Segments**.
* A **LiveInterval** is a subclass of **LiveRange** and is specialized to
  represent liveness for registers (virtual and physical).

