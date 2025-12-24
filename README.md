# PSS-Sketch: A Memory-efficient Sketch Framework for Persistent Super-spreader Detection in High-speed Data Streams

This is the repository for source codes of PSS-Sketch.

## Introduction
Persistent super-spreader detection is essential for fine-grained network monitoring and data management in high-speed data streams. Existing methods mainly focus on identifying super-spreaders within a single measurement period, neglecting their temporal persistence. This paper introduces PSS-Sketch, a memory-efficient framework designed for fast and accurate detection of persistent super-spreaders. Our design is driven by the observation that most flows are transient and often arrive in short bursts, whereas persistent super-spreaders remain active across multiple periods. We validate this observation empirically and embed it into our probabilistic update strategy, providing better protection for persistent super-spreaders.
We further establish rigorous theoretical guarantees, deriving tight error bounds for PSS-Sketch, and implement the framework on both CPU and GPU platforms. Extensive experiments on four real-world datasets demonstrate that PSS-Sketch significantly outperforms state-of-the-art sketches in terms of accuracy, while achieving high throughput and low restoring latency.

## About this repo

- `CPU Implementation` contains source code for C++ implementations of PSS-Sketch and related works.
- `GPU Implementation` contains the source code for CUDA implementation.
