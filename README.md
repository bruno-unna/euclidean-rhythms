[![Build status](https://github.com/bruno-unna/euclidean-rhythms/actions/workflows/build.yml/badge.svg)](https://github.com/bruno-unna/euclidean-rhythms/actions)
[![GitHub Release](https://img.shields.io/github/v/release/bruno-unna/euclidean-rhythms)](https://github.com/bruno-unna/euclidean-rhythms/releases/latest)
[![GitHub contributors](https://img.shields.io/github/contributors/bruno-unna/euclidean-rhythms)](https://github.com/bruno-unna/euclidean-rhythms/graphs/contributors)

# Euclidean rhythms

An implementation of the Euclidean rhythms idea in the form of plugins.

## Table of contents

1. [Introduction](#introduction)
2. [Background](#background)
3. [Algorithm](#algorithm)
    1. [Notation](#notation)
    2. [Desktop runs](#desktop-runs)
4. [Plugins](#plugins)
    1. [LV2](#lv2)
        1. [Description](#description)
        2. [How to build](#how-to-build)
5. [Conventions](#conventions)
6. [Acknowledgements](#acknowledgements)
7. [Licence & copyright](#licence--copyright)

## Introduction

The idea for this project came to me when I
read [someone asking for help](https://discourse.ardour.org/t/euclidean-rhythms/107461) to produce Euclidean rhythms in
Ardour. After realising how fascinating the idea is, and also that most implementations are commercial, and closed
source, I decided to tackle this very need, in what I believe is the best possible way: by providing a FOSS
implementation.

I have taken inspiration from Wouter
Hisschemöller's [music pattern generator](https://github.com/hisschemoller/music-pattern-generator), but his approach
and objective are different to what this project intends to do. I'd say that the audience for that project is different
to this one. Also, that project unfortunately seems to be receiving less attention these days (as of August 2022 their
latest release is from 2020).

## Background

In 2004 [Godfried Toussaint](https://en.wikipedia.org/wiki/Godfried_Toussaint)
published [a seminal paper](http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf) in which he showed that the
Euclidean algorithm for computing the greatest common divisor of two numbers can be used to generate many of the most
important traditional rhythms of the world. He dubbed them _Euclidean rhythms_ in his paper.

The core idea is to distribute, as evenly as possible, a number of onsets (attacks, or notes) unto a number of beats.
Examples:

- How can four onsets be distributed in a pattern of sixteen beats? That one is trivial: an onset every four beats:
  ```
  E(4, 16) = [x . . . x . . . x . . . x . . . ]
  ```
- More interestingly: how to distribute, say three onsets over a pattern of eight beats? One way to do it would be:
  ```
  E(3, 8) = [x . . x . . x .]
  ```
  and the other ways would simply be rotations of this one.

The objective of this project is to produce a FOSS plugin that can be introduced in the processing chain of
a [digital audio workstation](https://en.wikipedia.org/wiki/Digital_audio_workstation) to produce MIDI events using the
Euclidean algorithm.

## Algorithm

Toussaint cited
[E. Bjorklund as the author](https://www.semanticscholar.org/paper/The-Theory-of-Rep-Rate-Pattern-Generation-in-the-Bjorklund/c652d0a32895afc5d50b6527447824c31a553659)
of the algorithm that he applied to music. In Toussaint's paper the algorithm is not explicitly stated, but it is
exemplified. And he asserts that the algorithm has a strong resemblance to Euclid's method for finding the GCD of two
integers, hence the name he chose for his idea.

### Notation

There are several ways to represent the result of calculating the Euclidean rhythm *E(k, n)* for a given number of
onsets (parameter *k*) and number of beats (parameter *n*).

#### Fully extended

In the examples above a fully expanded notation was used, where each beat can be represented as an *x* (if it's an
onset), or as *.* if it's a silence:

```
E(4, 16) = [x . . . x . . . x . . . x . . . ]
```

```
E(3, 8) = [x . . x . . x .]
```

#### Binary

Those can be easily replaced with *1* and *0*, respectively, to make it more compact (and potentially more machine
friendly). That would yield *E(4, 16) = [1000100010001000]* and *E(3, 8) = [10010010]*.

#### Adjacent-inter-onset-interval vector

There is, however, an even more compact notation. It consists of a vector with the size of the intervals between the
onsets. For the same examples above, they would be *E(4, 16) = (4444)* and *E(3, 8) = (332)*.

### Desktop runs

#### E(1, 4)

A very simple case, in binary notation. In this case, *k=1*, *n=4*. That means that there are silent *p=3* beats.

Let's call *g* the groups of onset-interval vectors that are produced by the algorithm. Let's call *r* the remaining
groups. Let's initialise *g* as a sequence of *k* *1*'s, and *r* as a sequence of *p* *0*'s.

```
1. g = {1}, r = {0, 0, 0}
2. g = {10}, r = {0, 0}
3. g = {100}, r = {0}
```

In step 1, *|g| <= |r|*  means that we can append one element of *r* to each element of *g*, to obtain a new *g* (with
the same number of elements, but each of them longer) and a new *r* (with less elements).

In step 2, the process is repeated, because once again *|g| <= |r|*.

In step 3, although it is still the case that *|g| <= |r|*, it is also the case that *|r| <= 1*, which is our exit
condition. All that is left to do is concatenate all elements of *g* (just a single *100* in our case) with the
remaining element in *r* (*0* in our case). And we say that *e(1, 4) = 1000*.

#### E(3, 8)

In this example *n=8* and *k=3*, so *p=5*.

```
1. g = {1, 1, 1}, r = {0, 0, 0, 0, 0}
2. g = {10, 10, 10}, r = {0, 0}
3. g = {100, 100}, r = {10}
```

In step 1, *|g| <= |r|* means that each *g* must receive an instance of the elements in *r*.

Step 2 is more interesting, because now there are not enough elements in *r* for all elements in *g*! What we do in this
case is split *g* in two: the elements that received a suffix from the previous *r*, and the elements that didn't. The
former become the new *g*, and the latter become the new *r*.

In step 3, *|r| <= 1*, which is our exit condition. So we can conclude that *e(3, 8) = 10010010*.

At each step, all elements in both *g* and *r* are uniform. Hence, a more compact notation is possible. Let's define the
ordered pair *(n, s)* as a sequence of *n* elements, all of them equal to *s*. Using that device, we can re-examine the
previous example:

```
1. g=(3, 1), r=(5, 0)
2. g=(3, 10), r=(2, 0)
3. g=(2, 100), r=(1, 10)
```

The last row gives directly our result:

```
e(3, 8) = g(2, 100) + r(1, 10) = 10010010
```

## Plugins

Because I do my music-related work [in Ardour](https://ardour.org/), in Linux, the first implementation that I have in
mind uses the LV2 format. If the community finds it interesting to have the plugin in other formats (e.g. VST), that
would be fantastic! But that's something I would appreciate some expert taking ownership of.

### LV2

#### Description

Source code is under `src/plugins`, in file `plugin.c`. The _turtle_ files are under `src/lv2ttl`.

#### How to build

The project uses meson to build. So you will need meson, ninja, and gcc. Also, the LV2 libraries. Starting with release
0.1.1, there is an environment file `environments/linux64.ini` that should be used when setting up meson's build
directory:

```
meson setup --native-file=environments/linux64.ini builddir
```

If that succeeds, installation is as simple as:

```
cd builddir
meson install
```

This will install the plugin to directory `/usr/local/lib/lv2`, which is conventional. You might need to have escalated
privileges to do this. If you don't have them, you can change the installation directory by adding the
option `--libdir=~/.lv2` to the first step.

## Conventions

Not many, but very important. I would appreciate anyone contributing to the project to follow them:

- Comment your commits [the right way](https://cbea.ms/git-commit/).
- When reviewing pull requests, write [useful comments](https://conventionalcomments.org/).

## Acknowledgements

Kudos to [David Robillard](mailto:d@drobilla.net) for all the work he's put into divulging and documenting LV2. Much of
the code of this plugin finds its embryo in David's examples.

Thanks to [Robin Gareus](mailto:robin@gareus.org) for sharing generously the source code of his own plugins with the
community. I've learnt a lot.

Thanks as well to JetBrains, for demonstrating their [commitment to Open Source](https://jb.gg/OpenSourceSupport) by
granting me a licence to use their IDEs for free for this project.

My deepest gratitude to the FOSS community in general. I humbly hope that this project serves as a small retribution for
all the good things that I've received.

## Licence & copyright

Copyright 2023 by Bruno Unna.

Euclidean Rhythms is free software: you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Euclidean Rhythms is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along with Euclidean Rhythms.
If not, see <https://www.gnu.org/licenses/>.
