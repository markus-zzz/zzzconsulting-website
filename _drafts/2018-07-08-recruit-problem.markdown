---
layout: post
title:  "A recruitment problem"
date:   2018-07-08 14:00:50 +0200
categories: jekyll update
---
## Welcome
As part of a recent hiring process I was asked to take a test that included the following problem. For various reasons I ended up not actually taking the test but since I had already been introduced to the problem ... I would now like to attempt to solve it.

### Problem
Four friends (A,B,C and D) live on a grid where only horizontal and vertical moves are allowed. Part of the grid is covered by solid obstacles, other parts are covered by snow and yet other parts are not covered at all. Our friends like to establish routes connecting their four houses (A,B,C and D) and by doing so they may have to shovel snow. Find a way to minimize the total amount of snow shoveled!

Example

Legend
- o - snow
- x - obstacle

```
oo ooooo
oAxooooo
oox oBoo
ooxooooo
ooo   oo
Dooo  oo
xxxooooo
ooCooooo
```
### Solution

Represent the map with a weighted graph, use Dijkstra's algorithm to find the least costly path between the houses. But will this be minimal in a global sense?
