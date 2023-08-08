#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : adapter.py


import network_gym_client.adapter
import sys
from gymnasium import spaces
import numpy as np

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
        self.env = "qos_steer"
        self.num_features = 3
        self.num_users = int(self.config_json['env_config']['num_users'])
        self.end_ts = 0
        if config_json['env_config']['env'] != self.env:
            sys.exit("[ERROR] wrong environment helper. config file environment: " + str(config_json['env_config']['env']) + " helper environment: " + str(self.env))

    def get_action_space (self):
        """Get action space for qos_steer env.

        Returns:
            spaces: action spaces
        """

        myarray = np.empty([self.num_users,], dtype=int)
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
                                            shape=(self.num_features, self.num_users), dtype=np.float32)

    def get_observation(self, df):
        """Prepare observation for qos_steer env.

        This function should return the same number of features defined in the :meth:`get_observation_space`.

        Args:
            df (pd.DataFrame): network stats measurement

        Returns:
            spaces: observation spaces
        """
        print (df)
        if not df.empty:
            self.end_ts = int(df['end_ts'][0])
        #data_recv_flat = df.explode(column=['user', 'value'])
        #print(data_recv_flat)

        df_rate = df[df['name'] == 'rate'].reset_index(drop=True) # get the rate
        df_rate = df_rate[df_rate['cid'] == 'All'].reset_index(drop=True).explode(column=['user', 'value']) #keep the flow rate.
        #print(df_rate)

        df_max_rate = df[df['name'] == 'max_rate'].reset_index(drop=True)
        df_phy_lte_max_rate = df_max_rate[df_max_rate['cid'] == 'LTE'].reset_index(drop=True).explode(column=['user', 'value']) #get the LTE max_rate
        df_phy_wifi_max_rate = df_max_rate[df_max_rate['cid'] == 'Wi-Fi'].reset_index(drop=True).explode(column=['user', 'value']) # get the Wi-Fi max rate

        #print(df_phy_lte_max_rate)
        #print(df_phy_wifi_max_rate)

        df_split_ratio = df[df['name'] == 'split_ratio'].reset_index(drop=True)
        df_wifi_split_ratio = df_split_ratio[df_split_ratio['cid'] == 'Wi-Fi'].reset_index(drop=True).explode(column=['user', 'value']) # get the Wi-Fi split ratio
        #print(df_wifi_split_ratio)

        # if not empty and send to wanDB database
        self.wandb_log_buffer_append(self.df_to_dict(df_phy_wifi_max_rate, "max-wifi-rate"))
    
        self.wandb_log_buffer_append(self.df_to_dict(df_phy_lte_max_rate, "max-lte-rate"))

        dict_rate = self.df_to_dict(df_rate, 'rate')
        dict_rate["sum_rate"] = df_rate[:]["value"].sum()
        self.wandb_log_buffer_append(dict_rate)

        self.wandb_log_buffer_append(self.df_to_dict(df_wifi_split_ratio, "wifi-split-ratio")) 
        
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
        """Prepare network policy for qos_steer env.

        Args:
            action (spaces): action from RL agent

        Returns:
            json: network policy
        """

        if action.size != self.num_users:
            sys.exit("The action size: " + str(action.size()) +" does not match with the number of users:" + self.num_users)
        # you may also check other constraints for action... e.g., min, max.

        # you can add more tags
        tags = {}
        tags["end_ts"] = self.end_ts
        tags["downlink"] = self.config_json["env_config"]["downlink"]
        tags["cid"] = 'Wi-Fi'

        # this function will convert the action to a nested json format
        policy1 = self.get_nested_json_policy('split_ratio', tags, action)
        
        tags["cid"] = 'LTE'
        policy2 = self.get_nested_json_policy('split_ratio', tags, (1-action))

        policy = policy1 + policy2
        print('Action --> ' + str(policy))
        return policy

    def get_reward(self, df):
        """Prepare reward for qos_steer env.

        Args:
            df_list (list[pandas.DataFrame]): network stats measurement

        Returns:
            spaces: reward space
        """
        df_qos_rate = df[df['name'] == 'qos_rate'].reset_index(drop=True)
        df_wifi_qos_rate = df_qos_rate[df_qos_rate['cid'] == 'Wi-Fi'].reset_index(drop=True).explode(column=['user', 'value']) # get the Wi-Fi qos_rate
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
        reward = np.sum(qos_rate>0.1) #we assume the min qos rate is 0.1 mbps
        return reward