#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : adapter.py

import network_gym_client.adapter

import sys
from gymnasium import spaces
import numpy as np
import math
import time
import pandas as pd
import json
from pathlib import Path

class Adapter(network_gym_client.adapter.Adapter):
    """nqos_split env adapter.

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

    def get_action_space(self):
        """Get action space for the nqos_split env.

        Returns:
            spaces: action spaces
        """
        return spaces.Box(low=0, high=1,
                                        shape=(self.size_per_feature,), dtype=np.float32)

    #consistent with the get_observation function.
    def get_observation_space(self):
        """Get the observation space for nqos_split env.

        Returns:
            spaces: observation spaces
        """

        return spaces.Box(low=0, high=1000,
                                            shape=(self.num_features, self.size_per_feature), dtype=np.float32)
    
    def get_observation(self, df):
        """Prepare observation for nqos_split env.

        This function should return the same number of features defined in the :meth:`get_observation_space`.

        Args:
            df (pd.DataFrame): network stats measurement

        Returns:
            spaces: observation spaces
        """
        #print (df)


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
                    self.action_data_format = row
                    rate_value = row['value']
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
        """Prepare policy for the nqos_split env.

        Args:
            action (spaces): action from the RL agent

        Returns:
            json: network policy
        """

        # you may also check other constraints for action... e.g., min, max.
        # make sure you convert the action to list, e.g., using tolist(), before adding to policy["value"].

        policy1 = json.loads(self.action_data_format.to_json())
        policy1["name"] = "wifi::dl::split_ratio"
        policy1["value"] = action.tolist() #convert to list type

        policy = policy1
        print('Action --> ' + str(policy))
        return policy

    def get_reward(self, df):
        """Prepare reward for the nqos_split env.

        Args:
            df (pd.DataFrame): network stats

        Returns:
            spaces: reward spaces
        """

        df_rate = None
        df_owd = None
        for index, row in df.iterrows():
            if row['source'] == 'gma':
                if row['name'] == 'dl::rate':
                    df_rate = row
                elif row['name'] == 'dl::owd':
                    df_owd = row
        
        ave_rate = np.mean(df_rate["value"])
        avg_delay = np.mean(df_owd["value"])
        max_delay = np.max(df_owd["value"])

        reward = 0
        if self.config_json["rl_config"]["reward_type"] == "utility":
            reward = self.netowrk_util(ave_rate, avg_delay)
        else:
            sys.exit("[ERROR] reward type not supported yet")

        self.wandb_log_buffer_append(self.df_to_dict(df_owd))
        self.wandb_log_buffer_append({"reward": reward, "avg_delay": avg_delay, "max_delay": max_delay})

        return reward

    def netowrk_util(self, throughput, delay, alpha=0.5):
        """
        Calculates a network utility function based on throughput and delay, with a specified alpha value for balancing. Default Reward function.
        
        Args:
            throughput: a float representing the network throughput in bits per second
            delay: a float representing the network delay in seconds
            alpha: a float representing the alpha value for balancing (default is 0.5)
        
        Returns:
            a float representing the alpha-balanced metric
        """
        # Calculate the logarithm of the delay in milliseconds
        log_delay = -10
        if delay>0:
            log_delay = math.log(delay)

        # Calculate the logarithm of the throughput in mb per second
        log_throughput = -10
        if throughput>0:
            log_throughput = math.log(throughput)

        #print("delay:"+str(delay) +" log(owd):"+str(log_delay) + " throughput:" + str(throughput)+ " log(throughput):" + str(log_throughput))
        
        # Calculate the alpha-balanced metric
        alpha_balanced_metric = alpha * log_throughput - (1 - alpha) * log_delay

        alpha_balanced_metric = np.clip(alpha_balanced_metric, -10, 10)
        
        return alpha_balanced_metric