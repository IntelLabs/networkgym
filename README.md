# NetworkGym Client

📋 **[NetworkGym Docs Website](https://intellabs.github.io/networkgym)**

📧 **[Contact Us](mailto:netaigym@gmail.com)**

The NetworkGym Client stands as a Python-centric client library created for NetworkGym, an innovative Simulation-as-a-Service framework crafted to democratize network AI research and development. This Client establishes a remote connection to the NetworkGym Server/Environment hosted on the cloud, facilitating agent training.
At present, Network Gym Client supports three environments: `nqos_split`, `qos_steer`, and `network_slicing`.

## 📚 Class Structure

This repository includes the network_gym_client components. The network_gym_server and network_gym_env components are hosted in our vLab machines. After cloning this repository, users can launch the network_gym_client to remotely connects to the newtork_gym_server and newtork_gym_env via the northbound interface.

- network_gym_client
  - gymnasium.env: *a customized gymnasium environment that communicates with the agent.*
  - adapter: *transform the network stats measurements to obs and reward; translate action to policy that can be applied to the network.*
  - northbound_interface: *communicates network confiugration, network stats and policy between client and network_gym server/environment.*
- agent: any gymnasium compatible agent.


## ⌛ Installation:
- Clone this repo.
- (Optional) Create a new virtual python environment.
```
python3 -m venv network_venv
source network_venv/bin/activate
```
- Install Required Libraries `pip install -r requirements.txt` or:
```
pip install gymnasium
pip install pandas
pip install pyzmq
pip install wandb
pip install tensorboard
```
- Request access to the [vLab](https://registration.intel-research.net/) machine.

## 🔗 Setup Port Forwarding to vLab Server:

**Skip this section if you plan to deploy the client on the mlwins-v01 vlab server.** Otherwise, follow the following steps to set up port forwarding from you local machine to the vLab server.
- First, setup port forwarding from the local port 8088 to the mlwins-v01 external server port 8088 via the SSH gateway using the following command in a screen session, e.g., `screen -S port8088`.
``` 
ssh -L 8088:mlwins-v01.research.intel-research.net:8088 ssh.intel-research.net
```
- If the previous command does not work, add your user account before the `ssh.intel-research.net` as follows.
```
ssh -L 8088:mlwins-v01.research.intel-research.net:8088 [YOUR_USER_NAME]@ssh.intel-research.net
```
 - If the previous command also does not work, add the following instructions to your ssh configure file, replace **[YOUR_USER_NAME]** with your user name and update **[PATH_TO_SSH]** accordingly.
```
# COMMAND: ssh mlwins

Host gateway
  HostName ssh.intel-research.net
  User [YOUR_USER_NAME]
  Port 22
  IdentityFile /home/[PATH_TO_SSH]/.ssh/id_rsa

Host mlwins
  HostName mlwins-v01.research.intel-research.net
  User [YOUR_USER_NAME]
  Port 22
  IdentityFile /home/[PATH_TO_SSH]/.ssh/id_rsa
  ProxyJump gateway
  LocalForward 8088 localhost:8088
```

## 🚀 Start NetworkGym Client:

- Update the common configuration file [common_config.json](network_gym_client/common_config.json). Go to the ⚙️ Configurable File Format Section for more details.

- Update the environment depend configuration file [network_gym_client/envs/qos_steer/config.json](network_gym_client/envs/qos_steer/config.json) or [network_gym_client/envs/nqos_split/config.json](network_gym_client/envs/nqos_split/config.json) or [network_gym_client/envs/network_slicing/config.json](network_gym_client/envs/network_slicing/config.json)


- Start the demo client using the following command:
```
python3 start_client_demo.py
```
- When the program terminates, visualize the output using the returned WanDB website. If the python program stops after sending out the start request as shown in the following, check if the port fowarding is broken.
```
...
test-0 started
test-0 Sending GMASim Start Request…
```

## 📁 File Structure:

```
📦 NetworkGym
┣ 📜 start_client_demo.py
┗ 📂 network_gym_client
  ┣ 📜 adapter.py (➡️ WanDB)
  ┣ 📜 common_config.json
  ┣ 📜 env.py
  ┣ 📜 northbound_interface.py (➡️ network_gym_server and network_gym_env)
  ┗ 📂 envs
    ┗ 📂 [ENV_NAME]
      ┣ 📜 adapter.py
      ┗ 📜 config.json
```

- Excuting the 📜 start_client_demo.py file will start a new simulation. To change the environment, modify the `env_name` parameter, e.g., `'nqos_split'`, `'qos_steer'`, or `'network_slicing'`. The 📜 common_config.json is used in all environments. Depends on the selected environments, the 📜 config.json and 📜 adapter.py in the [ENV_NAME] folder will be loaded. The 📜 adapter.py helps preparing observations, rewards and actions for the selected environment.
- The 📜 start_client_demo.py create a Network Gym environment, which remotely connects to the ns-3 based Network Gym Simualtor (hosted in vLab machine) using the 📜 northbound_interface. 📜 start_client_demo.py also uses random samples from the action space to interact with the Network Gym environment. The results are synced to ➡️ WanDB database. We provide the following code snippet from the 📜 start_client_demo.py as an example:

```python
#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_client_demo.py

from network_gym_client import load_config_file
from network_gym_client import Env as NetworkGymEnv

client_id = 0
env_name = "nqos_split"
config_json = load_config_file(env_name)
config_json["rl_config"]["agent"] = "random"
# Create the environment
env = NetworkGymEnv(client_id, config_json) # make a network env using pass client id and configure file arguements.

num_steps = 1000
obs, info = env.reset()

for step in range(num_steps):

    action = env.action_space.sample()  # agent policy that uses the observation and info
    obs, reward, terminated, truncated, info = env.step(action)

    # If the environment is end, exit
    if terminated:
        break

    # If the epsiode is up (environment still running), then start another one
    if truncated:
        obs, info = env.reset()
```
 
## ⚙️ Configurable File Format:
- [common_config.json](network_gym_client/common_config.json)

```json
{
  "algorithm_client_port": 8088,//do not change
  "session_name": "test",//Make sure to change the "session_name" to your assgined session name. Cannot use '-' in the name!!!!
  "session_key": "test",//Make sure to change the "session_key" to your assgined keys.
}
```
- [network_gym_client/envs/qos_steer/config.json](network_gym_client/envs/qos_steer/config.json) or [network_gym_client/envs/nqos_split/config.json](network_gym_client/envs/nqos_split/config.json) or [network_gym_client/envs/network_slicing/config.json](network_gym_client/envs/network_slicing/config.json)
```json
{
  //never use negative value for any configure vale!!!
  "env_config":{
      "type": "env-start", //do not change
      "steps_per_episode": 10, //the env will run app_and_measurement_start_time_ms + (measurement_interval_ms+measurement_guard_interval_ms) * steps_per_episode * episodes_per_session
      "episodes_per_session": 5,
      "random_seed": 2, //change the random seed for this simulation run
      "downlink": true, //set to true to simulate downlink data flow, set to false to simulate uplink data flow.
      "max_wait_time_for_action_ms": -1, //the max time the network gym worker will wait for an action. set to -1 will cap the wait time to 100 seconds.
      "enb_locations":{//x, y and z locations of the base station, we support 1 base station only
        "x":40,
        "y":0,
        "z":3
      },
      "ap_locations":[//x, y and z location of the Wi-Fi access point, add or remove element in this list to increase or reduce AP number. We support 0 AP as well.
        {"x":30,"y":0,"z":3},
        {"x":50,"y":0,"z":3}
      ],
      "num_users" : 4,
      "slice_list":[ //network slicing environment only, resouce block group (rbg) size maybe 1, 2, 3 or 4, it depends on the resource block num, see table 7.1.6.1-1 of 36.213
        {"num_users":5,"dedicated_rbg":2,"prioritized_rbg":3,"shared_rbg":4},
        {"num_users":5,"dedicated_rbg":5,"prioritized_rbg":6,"shared_rbg":7},
        {"num_users":5,"dedicated_rbg":0,"prioritized_rbg":0,"shared_rbg":100}
      ],
      "user_random_walk":{ // configure random walk model with Distance mode. https://www.nsnam.org/docs/release/3.16/doxygen/classns3_1_1_random_walk2d_mobility_model.html
        "min_speed_m/s": 1, //A random variable used to pick the min random walk speed (m/s). Set min and max speed to 0 to disable random walk
        "max_speed_m/s": 2, //A random variable used to pick the max random walk speed (m/s). Set min and max speed to 0 to disable random walk
        "min_direction_gradients": 0.0, //A random variable used to pick the min random walk direction (gradients). [Min=0.0|Max=6.283184]
        "max_direction_gradients": 6.283184, //A random variable used to pick the max random walk direction (gradients). [Min=0.0|Max=6.283184]
        "distance_m": 3 //change current direction and speed after moving for this distance (m)
      },
      "user_location_range":{//initially, users will be randomly deployed within this x, y range. if user_random_walk_max_speed_m/s > 0, the user will random walk within this boundary.
        "min_x":0,
        "max_x":80,
        "min_y":0,
        "max_y":10,
        "z":1.5
      },
      "app_and_measurement_start_time_ms": 1000, //when the application starts traffic and send measurement to RL agent
      "transport_protocol": "tcp", //"tcp" or "udp"
      "udp_poisson_arrival": false, // if "transport_protocol" is "udp", this para controls whether the generater using poisson process.
      "min_udp_rate_per_user_mbps": 2, // if "transport_protocol" is "udp", this para controls the min sending rate.
      "max_udp_rate_per_user_mbps": 3, // if "transport_protocol" is "udp", this para controls the max sending rate.
      "respond_action_after_measurement": true, //do not change
      "qos_requirement": {//only for qos_steer environment
        "test_duration_ms": 500,//duration for qos testing
        "delay_bound_ms": 100,//max delay for qos flow
        "delay_test_1_thresh_ms": 200, //only for delay violation test 1 measurement, this para does not impact flow qos.
        "delay_test_2_thresh_ms": 400, //only for delay violation test 2 measurement, this para does not impact flow qos.
        "delay_violation_target":0.02, //delay violation target for qos flow
        "loss_target": 0.001 //loss target for qos flow
      },
      "GMA": {
          "downlink_mode": "auto", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
          "uplink_mode": "auto", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
          "measurement_interval_ms": 100, //duration of a measurement interval.
          "measurement_guard_interval_ms": 0 //gap between 2 measurement interval
        },
        "Wi-Fi": {
          "ap_share_same_band": false, //set to true, ap will share the same frequency band.
          "enable_rx_signal_based_handover": false, //Always connect to the Wi-Fi AP with strongest rx signal, the rx signal is measured from BEACONS.
          "measurement_interval_ms": 100,
          "measurement_guard_interval_ms": 0
        },
        "LTE": {
          "qos_aware_scheduler": true, //qos_steer environment only, set to true to enable qos aware scheduler for LTE.
          "resource_block_num": 25, //number of resouce blocks for LTE, 25 for 5 MHZ, 50 for 10 MHZ, 75 for 15 MHZ and 100 for 20 MHZ.
          "measurement_interval_ms": 100,
          "measurement_guard_interval_ms": 0
        }
      },

  "rl_config":{//see the following for config options 
    "agent": "",
    "reward_type" : "utility"
  },

  "rl_config_option_list"://do not change this list, it provides the available inputs for the rl_config
  {
    "agent": [""],// set to "" will disable agent, i.e., use the system's default algorithm for offline data collection
    "reward_type" : ["utility"]
  },

  "action_template":{//do not change
  }
}
```
