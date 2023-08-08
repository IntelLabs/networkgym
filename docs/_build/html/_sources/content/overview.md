---
title: Overview
---
# Overview

## NetworkGym: Components and Interfaces

The NetworkGym framework consists of three essential components - the **Client**, the **Server**, and the **Environment** - each playing a crucial role in the system. Additionally, it encompasses two interfaces - the **Northbound** and the **Southbound interfaces**, facilitating seamless communication and interaction within the framework.

The graphical representation below illustrates the architecture of NetworkGym:


```{mermaid}
flowchart TB

subgraph network_gym_server
northbound <--> southbound[[southbound_interface]]
end

subgraph network_gym_env
southbound_interface
simulator
emulator
testbed 
end

agent <--> gymnasium.env

gymnasium.env -- action --> adapter
adapter -- obs,rewards --> gymnasium.env

subgraph network_gym_client
gymnasium.env
adapter
northbound_interface[[northbound_interface]]
end



adapter --policy--> northbound_interface
northbound_interface --network_stats--> adapter


northbound_interface --env_config,policy--> northbound[[northbound_interface]]
northbound --network_stats--> northbound_interface




southbound --env_config,policy--> southbound_interface[[southbound_interface]]
southbound_interface --network_stats--> southbound


click gymnasium.env "/api/network_gym_client/env.html" _blank
style gymnasium.env fill:#1E90FF,color:white,stroke:white

click adapter "/api/network_gym_client/adapter.html" _blank
style adapter fill:#1E90FF,color:white,stroke:white

click northbound_interface "/api/network_gym_client/northbound_interface.html" _blank
style northbound_interface fill:#1E90FF,color:white,stroke:white
```
In this diagram, you can see how the components and interfaces of NetworkGym work together to create a cohesive and efficient Simulation-as-a-Service framework for Network AI Research and Development.

## Client Component
The NetworkGymClient component comprises three main elements: the **Northbound Interface**, an **Adapter**, and a **customized Gymnasium environment**. The Northbound Interface establishes the connection between the client and the server. It enables the client to select and configure the desired network environment. The environment-specific Adapter is responsible for transforming the NetworkGym data format into the Gymnasium data format, allowing seamless communication with Gymnasium-compatible agents like stable-baselines3 and cleanRL.

## Server Component
The NetworkGymServer component plays a central role in communication between the client and the environment. It utilizes the **Northbound Interface** to interact with the client and the **Southbound Interface** to communicate with the environment. Additionally, the server maintains a routing map, which keeps track of each active client and its assigned environment during a connected session.

## Environment Component
The NetworkGym environment (either a simulator, emulator, or testbed) connects to the server through the **Southbound Interface**. At present, the framework supports the **ns-3** based simulator, offering **three** distinct network environments. Furthermore, it provides support for measuring **ten** different network statistics to enhance the simulation capabilities.
```{note}
Please note that the Southbound Interface is currently under development and will be released in the future.
```
## Class Diagram

```{mermaid}
classDiagram
network_gym_client *-- env
env *-- adapter
env *-- northbound_interface

northbound_interface -- network_gym_server
network_gym_server  -- southbound_interface

network_gym_env *-- southbound_interface

class env
env: +reset() obs, info
env: +step(action) obs, reward, ...

class network_gym_client

class network_gym_server
network_gym_server: +northbound_interface
network_gym_server: +southbound_interface

class network_gym_env

class northbound_interface
northbound_interface: +connect() void
northbound_interface: +send(policy) void
northbound_interface: +recv() network_stats

class adapter 
adapter : +prepare_obs(network_stats) obs
adapter : +prepare_reward(network_stats) reward
adapter : +prepare_policy(action) policy
```
