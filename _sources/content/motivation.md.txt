---
title: Motivation
---

# Motivation

## Network AI Models/Algorithms Development Cycle

```{figure} motivation.png
---
width: 100%
---
```
## Challenges Faced by Network AI Developers
1. real-world dataset controlled by network operator, difficult to acquire, not aligned with specific usage or requirement.
2. dataset by itself not enough, also need environment to train/test AI models, e.g., Reinforcement Learning, etc.
```{admonition} NetworkGym's Approach to Addressing this Challenge
✔️ Currently, NetworkGym environment enable 3 use cases: multi-access traffic splitting, QoS-aware traffic steering, and (cellular) RAN slicing.
```
3. network simulation tools (e.g., ns-3, etc.) often very complex and difficult to use, especially for Network AI researcher and developer.
```{admonition} NetworkGym's Approach to Addressing this Challenge
✔️ NetworkGym enables agent training without the requirement of network simulation expertise.
```
4. lack of common simulation environment with simple APIs to develop, evaluate, and benchmark Network AI models and algorithms.
```{admonition} NetworkGym's Approach to Addressing this Challenge
✔️ NetworkGym adheres to the standard gymnasium API for AI model training and additionally offers an API for network simulation configuration.
```