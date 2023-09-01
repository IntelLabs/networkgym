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
        self.action_max_value = 32
        self.num_features = 3
        self.num_users = int(self.config_json['env_config']['num_users'])
        self.end_ts = 0

        if config_json['env_config']['env'] != self.env:
            sys.exit("[ERROR] wrong environment Adapter. Configured environment: " + str(config_json['env_config']['env']) + " != Launched environment: " + str(self.env))

    def get_action_space(self):
        """Get action space for the nqos_split env.

        Returns:
            spaces: action spaces
        """
        return spaces.Box(low=0, high=1,
                                        shape=(self.num_users,), dtype=np.float32)

    #consistent with the get_observation function.
    def get_observation_space(self):
        """Get the observation space for nqos_split env.

        Returns:
            spaces: observation spaces
        """

        return spaces.Box(low=0, high=1000,
                                            shape=(self.num_features, self.num_users), dtype=np.float32)
    
    def get_observation(self, df):
        """Prepare observation for nqos_split env.

        This function should return the same number of features defined in the :meth:`get_observation_space`.

        Args:
            df (pd.DataFrame): network stats measurement

        Returns:
            spaces: observation spaces
        """
        #print (df)
        if not df.empty:
            self.end_ts = int(df['end_ts'][0])
        #data_recv_flat = df.explode(column=['user', 'value'])
        #print(data_recv_flat)

        df_rate = None
        df_phy_wifi_max_rate = None
        df_phy_lte_max_rate = None
        df_wifi_split_ratio = None
        df_x_loc = None
        df_y_loc = None
        for index, row in df.iterrows():
            if row['name'] == 'rate':
                if row['cid'] == 'All':
                    df_rate = row
            elif row['name'] == 'max_rate':
                if row['cid'] == 'LTE':
                    df_phy_lte_max_rate = row
                elif row['cid'] == 'Wi-Fi':
                    df_phy_wifi_max_rate = row
            elif row['name'] == 'split_ratio':
                if row['cid'] == 'Wi-Fi':
                    df_wifi_split_ratio = row
            elif row['name'] == 'x_loc':
                df_x_loc = row
            elif row['name'] == 'y_loc':
                df_y_loc = row

        #print(df_rate)
        #print(df_phy_lte_max_rate)
        #print(df_phy_wifi_max_rate)
        #print(df_wifi_split_ratio)
        #print(df_x_loc)
        #Print(df_y_loc)

        # if not empty and send to wanDB database
        self.wandb_log_buffer_append(self.df_to_dict(df_phy_wifi_max_rate, "max-wifi-rate"))
    
        self.wandb_log_buffer_append(self.df_to_dict(df_phy_lte_max_rate, "max-lte-rate"))

        dict_rate = self.df_to_dict(df_rate, 'rate')
        dict_rate["sum_rate"] = sum(df_rate["value"])
        self.wandb_log_buffer_append(dict_rate)

        self.wandb_log_buffer_append(self.df_to_dict(df_wifi_split_ratio, "wifi-split-ratio")) 
        
        self.wandb_log_buffer_append(self.df_to_dict(df_x_loc, "x_loc"))

        self.wandb_log_buffer_append(self.df_to_dict(df_y_loc, "y_loc"))
        
        # Fill the empy features with -1
        phy_lte_max_rate = self.fill_empty_feature(df_phy_lte_max_rate, -1)
        phy_wifi_max_rate = self.fill_empty_feature(df_phy_wifi_max_rate, -1)
        flow_rate = self.fill_empty_feature(df_rate, -1)

        observation = np.vstack([phy_lte_max_rate, phy_wifi_max_rate, flow_rate])

        # add a check that the size of observation equals the prepared observation space.
        if len(observation) != self.num_features:
            sys.exit("The size of the observation and self.num_features is not the same!!!")
        return observation

    def get_policy(self, action):
        """Prepare policy for the nqos_split env.

        Args:
            action (spaces): action from the RL agent

        Returns:
            json: network policy
        """

        if action.size != self.num_users:
            sys.exit("The action size: " + str(action.size()) +" does not match with the number of users:" + self.num_users)
        # you may also check other constraints for action... e.g., min, max.

        #scale action from [0, 1] to [0, self.action_max_value]

        scaled_action = np.interp(action, (0, 1), (0, self.action_max_value))
        scaled_action = np.round(scaled_action).astype(int) # force it to be an interger.

        # you can add more tags
        tags = {}
        tags["end_ts"] = self.end_ts
        tags["downlink"] = self.config_json["env_config"]["downlink"]
        tags["cid"] = 'Wi-Fi'

        # this function will convert the action to a nested json format
        policy1 = self.get_nested_json_policy('split_ratio', tags, scaled_action)
        
        tags["cid"] = 'LTE'
        policy2 = self.get_nested_json_policy('split_ratio', tags, (self.action_max_value-scaled_action))

        policy = [policy1, policy2]
        print('Action --> ' + str(policy))
        return policy

    def get_reward(self, df):
        """Prepare reward for the nqos_split env.

        Args:
            df (pd.DataFrame): network stats

        Returns:
            spaces: reward spaces
        """
        df_owd = None
        df_rate = None

        for index, row in df.iterrows():
            if row['name'] == 'rate':
                if row['cid'] == 'All':
                    df_rate = row
            elif row['name'] == 'owd':
                if row['cid'] == 'All':
                    df_owd = row
    
        ave_rate = np.mean(df_rate["value"])
        avg_delay = np.mean(df_owd["value"])
        max_delay = np.max(df_owd["value"])

        reward = 0
        if self.config_json["rl_config"]["reward_type"] == "utility":
            reward = self.netowrk_util(ave_rate, avg_delay)
        else:
            sys.exit("[ERROR] reward type not supported yet")

        self.wandb_log_buffer_append(self.df_to_dict(df_owd, "owd"))
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