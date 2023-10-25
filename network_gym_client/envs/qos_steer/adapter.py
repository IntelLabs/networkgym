#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : adapter.py


import network_gym_client.adapter
import sys
from gymnasium import spaces
import numpy as np
from pathlib import Path
import json

class Adapter(network_gym_client.adapter.Adapter):
    """qos_steer environment adapter.

    Args:
        Adapter (network_gym_client.adapter.Adapter): base class.
    """
    def __init__(self, config_json):
        """Initialize the adapter.

        Args:
            config_json (json): the configuration file
        """

        super().__init__(config_json)
        self.env = Path(__file__).resolve().parent.name
        self.num_features = 3
        self.size_per_feature = int(self.config_json['env_config']['num_users'])

        if config_json['env_config']['env'] != self.env:
            sys.exit("[ERROR] wrong environment Adapter. Configured environment: " + str(config_json['env_config']['env']) + " != Launched environment: " + str(self.env))

    def get_action_space (self):
        """Get action space for qos_steer env.

        Returns:
            spaces: action spaces
        """

        myarray = np.empty([self.size_per_feature,], dtype=int)
        myarray.fill(2)
        #print(myarray)
        return spaces.MultiDiscrete(myarray)

    #consistent with the get_observation function.
    def get_observation_space(self):
        """Get observation space for qos_steer env.

        Returns:
            spaces: observation spaces
        """
        return spaces.Box(low=0, high=1000,
                                            shape=(self.num_features, self.size_per_feature), dtype=np.float32)

    def get_observation(self, df):
        """Prepare observation for qos_steer env.

        This function should return the same number of features defined in the :meth:`get_observation_space`.

        Args:
            df (pd.DataFrame): network stats measurement

        Returns:
            spaces: observation spaces
        """
        #print (df)

        #data_recv_flat = df.explode(column=['user', 'value'])
        #print(data_recv_flat)

        df_rate = None
        df_wifi_traffic_ratio = None
        df_x_loc = None
        df_y_loc = None
        df_phy_wifi_max_rate = None
        df_phy_lte_max_rate = None

        rate_value = np.empty(self.size_per_feature, dtype=object)
        wifi_max_rate_value = np.empty(self.size_per_feature, dtype=object)
        lte_max_rate_value = np.empty(self.size_per_feature, dtype=object)

        for index, row in df.iterrows():
            if row['source'] == 'gma':
                if row['name'] == 'dl::rate':
                    df_rate = row
                    rate_value = row['value']
                    self.action_data_format = row
                elif row['name'] == 'wifi::dl::traffic_ratio':
                    df_wifi_traffic_ratio = row
                elif row['name'] == 'x_loc':
                    df_x_loc = row
                elif row['name'] == 'y_loc':
                    df_y_loc = row
            elif row['source'] == 'wifi':
                if row['name'] == 'dl::max_rate':
                    df_phy_wifi_max_rate = row
                    wifi_max_rate_value = row['value']
            elif row['source'] == 'lte':
                if row['name'] == 'dl::max_rate':
                    df_phy_lte_max_rate = row
                    lte_max_rate_value = row['value']
        
        #print(df_rate)
        #print(df_phy_lte_max_rate)
        #print(df_phy_wifi_max_rate)
        #print(df_wifi_split_ratio)
        #print(df_x_loc)
        #Print(df_y_loc)

        # if not empty and send to wanDB database
        self.wandb_log_buffer_append(self.df_to_dict(df_phy_wifi_max_rate))
    
        self.wandb_log_buffer_append(self.df_to_dict(df_phy_lte_max_rate))

        dict_rate = self.df_to_dict(df_rate)
        if df_rate is not None:
            dict_rate["sum_rate"] = sum(df_rate["value"])
        self.wandb_log_buffer_append(dict_rate)

        self.wandb_log_buffer_append(self.df_to_dict(df_wifi_traffic_ratio)) 
        
        self.wandb_log_buffer_append(self.df_to_dict(df_x_loc))

        self.wandb_log_buffer_append(self.df_to_dict(df_y_loc))

        observation = np.vstack([lte_max_rate_value, wifi_max_rate_value, rate_value])
        print('Observation --> ' + str(observation))
        return observation


    def get_policy(self, action):
        """Prepare network policy for qos_steer env.

        Args:
            action (spaces): action from RL agent

        Returns:
            json: network policy
        """
        # you may also check other constraints for action... e.g., min, max.

        policy1 = json.loads(self.action_data_format.to_json())
        policy1["name"] = "wifi::dl::split_ratio"
        policy1["value"] = action.tolist()

        print('Action --> ' + str(policy1))
        return policy1

    def get_reward(self, df):
        """Prepare reward for qos_steer env.

        Args:
            df (pd.DataFrame]): network stats measurement

        Returns:
            spaces: reward space
        """

        df_wifi_qos_rate = None
        for index, row in df.iterrows():
            if row['source'] == 'gma':
                if row['name'] == 'wifi::dl::qos_rate':
                    df_wifi_qos_rate = row
        #print (df_wifi_qos_rate)

        reward = 0
        if self.config_json["rl_config"]["reward_type"] == "wifi_qos_user_num":
            reward = self.calculate_wifi_qos_user_num(df_wifi_qos_rate[:]["value"])
        else:
            sys.exit("[ERROR] reward type not supported yet")

        self.wandb_log_buffer_append(self.df_to_dict(df_wifi_qos_rate, "wifi-qos-rate"))
        self.wandb_log_buffer_append({"reward": reward})

        return reward

    def calculate_wifi_qos_user_num(self, qos_rate):
        """Calculate the number of QoS users over Wi-Fi. Default reward function.

        Args:
            qos_rate (pandas.DataFrame): qos data rate per user

        Returns:
            double: reward
        """
        #print(qos_rate)
        reward = 0
        for r in qos_rate:
            if r > 0.1: #we assume the min qos rate is 0.1 mbps
                reward = reward + 1
        return reward