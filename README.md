# Euclidean rhythms

An implementation of the Euclidean rhythms idea in the form of plugins.

## Background

In 2004 [Godfried Toussaint](https://en.wikipedia.org/wiki/Godfried_Toussaint) published [a seminal paper](http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf) in which he showed that the Euclidean algorithm for computing the greatest common divisor of two numbers can be used to generate many of the most important traditional rhythms of the world. He dubbed them _Euclidean rhythms_ in his paper.

The core idea is to distribute, as evenly as possible, a number of onsets (attacks, or notes) unto a number of beats. Examples:
- How can four onsets be distributed in a pattern of sixteen beats? That one is trivial: an onset every four beats:
  ```
  E(4, 16) = [x . . . x . . . x . . . x . . . ]
  ```
- More iterestingly: how to distribute, say three onbeats over a pattern of eight beats? One way to do it would be:
  ```
  E(3, 8) = [x . . x . . x .]
  ```
  and the other ways would simply be rotations of this one.

The objective of this project is to produce a FOSS plugin that can be introduced in the processing chain of a [digital audio workstation](https://en.wikipedia.org/wiki/Digital_audio_workstation) to produce MIDI events using the Euclidean algorithm.

## Algorithm

Toussaint cited [E. Bjorklund as the author](https://www.semanticscholar.org/paper/The-Theory-of-Rep-Rate-Pattern-Generation-in-the-Bjorklund/c652d0a32895afc5d50b6527447824c31a553659) of the algorithm that he applied to music. In Toussaint's paper the algorithm is not explicitly stated, but it is exemplified. And he asserts that the algorithm has a strong resemblance to Euclid's method for finding the GCD of two integers, hence the name he chose for his idea.

## Plugin(s)

The idea for this project came to me when I read [someone asking for help](https://discourse.ardour.org/t/euclidean-rhythms/107461) to produce Euclidean rhythms in Ardour. After realising how fascinating the idea is, and also that most implementations are commercial, and closed source, I decided to tackle this very need,the best possible way: by provided a FOSS implementation.

I have taken inspiration from Wouter Hisschemöller's [music pattern generator](https://github.com/hisschemoller/music-pattern-generator), but his approach and objective is different to what this project intends to do. I'd say that the audience for that project is different to this one. Also, that project unfortunately seems to be receiving less attention these days (as of August 2022 their latest release is from 2020).

Because I do my music-related work [in Ardour](https://ardour.org/), in Linux, the first implementation that I have in mind uses the LV2 format. If the community finds it interesting to have the plugin in other formats (e.g. VST), that would be fantastic! But that's something I would appreciate some expert taking ownership of.

## Conventions

Not many, but very important. I would appreciate anyone contributing to the project to follow them:
- Comment your commits [the right way](https://cbea.ms/git-commit/).
- When reviewing pull requests, write [useful comments](https://conventionalcomments.org/).