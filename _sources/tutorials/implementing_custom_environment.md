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

Establishing a connection with your custom environment is a straightforward process through the provided [demo](https://github.com/IntelLabs/networkgym/blob/main/start_client_demo.py). Adjust the env_name to "custom" as shown: `env_name = "custom"`. Then you can launch the client using the following command:

```python
python3 start_client_demo.py
```

The terminal should print the following info:
```
----------| step() at episode:2, step:10 |----------
Action --> [{'name': 'custom_action', 'custom_tag': 'Wi-Fi', '': {'user': [0, 1, 2, 3, 4], 'value': [0.9269856811, 0.5833047032, 0.6873341799, 0.0909367353, 0.3403381109]}}, {'name': 'custom_action', 'custom_tag': 'LTE', '': {'user': [0, 1, 2, 3, 4], 'value': [0.0730143189, 0.4166952968, 0.3126658201, 0.9090632796, 0.6596618891]}}]
          name  unit  start_ts  end_ts direction group    cid             user            value
0         rate  mbps     20000   21000        DL   GMA    All  [0, 1, 2, 3, 4]  [5, 4, 8, 5, 6]
1         rate  mbps     20000   21000        DL   GMA  Wi-Fi  [0, 1, 2, 3, 4]  [8, 8, 3, 9, 6]
2     qos_rate  mbps     20000   21000        DL   GMA  Wi-Fi  [0, 1, 2, 3, 4]  [4, 6, 7, 3, 5]
3         rate  mbps     20000   21000        DL   GMA    LTE  [0, 1, 2, 3, 4]  [8, 7, 5, 6, 6]
4     max_rate  mbps     20000   21000        DL   PHY    LTE  [0, 1, 2, 3, 4]  [4, 5, 4, 8, 8]
5     max_rate  mbps     20000   21000        DL   PHY  Wi-Fi  [0, 1, 2, 3, 4]  [4, 4, 6, 5, 3]
6  split_ratio           20000   21000        DL   GMA  Wi-Fi  [0, 1, 2, 3, 4]  [4, 7, 9, 4, 8]
7          owd    ms     20000   21000        DL   GMA    All  [0, 1, 2, 3, 4]  [4, 5, 4, 6, 3]
```

Presently, our support is limited to accessing custom environments launched by the same users. For instance, if you initiate a custom environment with the "session_name": "test", you can solely connect to this custom environment using the same "session_name": "test" when initializing the client.

## Substituting the Dummy Simulator

After successfully establishing a connection between your client and the custom environment, you're ready to replace the dummy simulator with your own proprietary simulator or testbed. Utilize the dummy simulator as a guide for communicating with the server through the Southbound Interface. Moreover, ensure that you modify the Adapter class in [envs/custom/adapter.py](https://github.com/IntelLabs/networkgym/blob/main/network_gym_client/envs/custom/adapter.py) to correspond with the measurements and tags produced by your own simulator or testbed.


## Releasing Your Custom Environment
After conducting your experiments within your custom environment, please get in touch with us at [netaigym@gmail.com](mailto:netaigym@gmail.com) to initiate the process of incorporating the environment into our servers.