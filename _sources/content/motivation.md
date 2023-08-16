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
## Network AI Developer's Challenges
1. real-world dataset controlled by network operator, difficult to acquire, not aligned with specific usage or requirement.
2. dataset by itself not enough, also need environment to train/test AI models, e.g., Reinforcement Learning, etc.
```{admonition} Why NetworkGym?
✔️ at present, NetworkGym environment enable 3 use cases: multi-access traffic splitting, QoS-aware traffic steering, and (cellular) RAN slicing.
```
3. network simulation tools (e.g., ns-3, etc.) often very complex and difficult to use, especially for Network AI researcher and developer.
```{admonition} Why NetworkGym?
✔️ working with NetworkGym requires zero knowledge of network simulation to train the agent.
```
4. lack of common simulation environment with simple APIs to develop, evaluate, and benchmark Network AI models and algorithms.
```{admonition} Why NetworkGym?
✔️ NetworkGym follows the standard gymnasium API for AI model training and provides additional API for network simulation configuration.
```