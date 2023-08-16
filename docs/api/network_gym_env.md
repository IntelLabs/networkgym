---
title: network_gym_env
---

# network_gym_env

```{toctree}
:hidden:
network_gym_env/configure
network_gym_env/dummy_sim

```

```{mermaid}
flowchart LR

subgraph network_gym_env
southbound_interface[[southbound_interface]]
southbound_interface <--> configure
southbound_interface <--> simulator
southbound_interface <-..-> emulator
southbound_interface <-..-> testbed
end

```
The NetworkGym Environment comprises two core components: the **Environment Configure** and the **NetworkGym Simulator**. These components establish a connection with the Server via the designated **SouthBound Interface**.

- The **SouthBound Interface** serves as the bridge between the environment and the server. It facilitates communication of network configurations, network statistics, and policies between the environment and the server.
- The **Environment Configure** module keeps the connection alive by periodically dispatching an "Env Hello" message to the server. Upon receiving a "Env Start" request, it disengages from the server and initiates the simulator.
- Upon activation, the ns-3 based **NetworkGym Simulator** establishes a connection with the server and instigates the exchange of measurements. Plans for incorporating an emulator and testbed alternative are earmarked for future release.

## NetworkGym UML Sequence Diagram

```{figure} network_gym_uml.png
---
width: 100%
---
```

The UML sequence diagram depicts the flow of messages for a complete cycle within a simulated network environment session. In this cycle, a Client initiates the process by sending an "Env Start" message to request the launch of a network environment. Upon receiving this message, the Env_Config component triggers the simulator to generate "Env Measurement" data and receive incoming "Action" message. Once the simulation concludes, the Env_Config component dispatches an "Env End" message to signal the termination of the Client-to-Env mapping. 

```{important}
only one of the components, Env_Config or Env_Sim, can establish a connection with the Server at any given moment, and these connections cannot occur simultaneously. From the Server's standpoint, it treats both the Env_Config and Env_Sim components equivalently, identifying them both as the Env entity.
```