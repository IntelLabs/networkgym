---
title: Implementing Custom Environment
---

# Implementing Custom Environment

```{figure} network_gym_overview2.png
---
width: 60%
---
```
NetworkGym provides a range of network Environments, it also supports Custom Environment via the SouthBound (SB) Interface.

We provide an example to build your own environment.
## start the custom environment.
The example environment we provide connects to the server, you do not need to provide any input as the env_list. Even if you use your own environment name, the server will overwite it to "session_name"+"-custom". e.g., for "test" account, the environemnt name will be "test-custom".

## Connects to your custom environment.
change the environment name to "custom", the server will automatically append your "session_name" in front of environment name "custom".


## Release your custmo environment to the public.
To release your environment for public usage, we will automatically