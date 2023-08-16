---
title: Implementing Custom Environment
---

# Implementing Custom Environment

```{figure} network_gym_overview2.png
---
width: 60%
---
```

NetworkGym offers a variety of official network environments and also streamlines the incorporation of third-party custom environments via the SouthBound (SB) Interface.

## Initiating the Custom Environment

Linking your environment to the server follows the same procedure as connecting the client to the server.

1. refer to the "Installation" section in the [networkgym](https://github.com/IntelLabs/networkgym) repository to install dependencies.
2. In the "Setup Port Forwarding to vLab Server" section, alter the port number to 8087. 
3. Adjust the values for "session_name" and "session_key" in the [network_gym_env/common_config.json](https://github.com/IntelLabs/networkgym/blob/main/network_gym_env/common_config.json) file according to your "session_name" and "session_key".

Once these steps are completed, you can launch your custom environment using the following command:
```python
python3 start_custom_env.py
```

The expected output in the terminal would be as follows:

```
test-1: env_config socket connected.
test-1: send env-hello msg.
{'type': 'env-hello', 'env_list': ['custom']}
test-2: env_config socket connected.
test-2: send env-hello msg.
{'type': 'env-hello', 'env_list': ['custom']}
```

```{note}
Please don't alter the names within the env_list. All third-party users are restricted to employing the environment name 'custom'.
```

## Connecting to Your Custom Environment

Establishing a connection with your custom environment is a straightforward process through the provided [demo](https://github.com/IntelLabs/networkgym/blob/main/start_client_demo.py). Adjust the env_name to "custom" as shown:

```python
env_name = "custom"
```

Presently, our support is limited to accessing custom environments launched by the same users. For instance, if you initiate a custom environment with the "session_name": "test", you can solely connect to this custom environment using the same "session_name": "test" when initializing the client.

## Releasing Your Custom Environment to the Public
After conducting your experiments within your custom environment, please get in touch with us to initiate the process of incorporating the environment into our servers.