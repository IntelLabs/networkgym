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
import plotext as plt
from rich.panel import Panel
from rich.layout import Layout
from rich.table import Table
from rich.columns import Columns

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

        num_users = 0
        for item in self.config_json['env_config']['per_slice_config']['num_users']:
            num_users += item
        self.config_json['env_config']['num_users'] = num_users
        
        self.size_per_feature = int(self.config_json['env_config']['num_users'])
        self.reward = 0
        if config_json['env_config']['env'] != self.env:
            sys.exit("[ERROR] wrong environment Adapter. Configured environment: " + str(config_json['env_config']['env']) + " != Launched environment: " + str(self.env))

    def get_action_space(self):
        """Get action space for the nqos_split env.

        Returns:
            spaces: action spaces
        """
        return spaces.Box(low=0, high=1,
                                        shape=(self.size_per_feature, 3), dtype=np.float32)

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
        #print (df.sort_values('name').to_string())

        df_rate = None
        df_wifi_rate = None
        df_lte_rate = None
        df_nr_rate = None

        df_wifi_traffic_ratio = None
        df_lte_traffic_ratio = None
        df_nr_traffic_ratio = None

        df_wifi_owd = None
        df_lte_owd = None
        df_nr_owd = None

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
                elif row['name'] == 'wifi::dl::rate':
                    df_wifi_rate = row
                elif row['name'] == 'lte::dl::rate':
                    df_lte_rate = row
                elif row['name'] == 'nr::dl::rate':
                    df_nr_rate = row
                elif row['name'] == 'wifi::dl::traffic_ratio':
                    df_wifi_traffic_ratio = row
                elif row['name'] == 'lte::dl::traffic_ratio':
                    df_lte_traffic_ratio = row
                elif row['name'] == 'nr::dl::traffic_ratio':
                    df_nr_traffic_ratio = row
                elif row['name'] == 'wifi::dl::owd':
                    df_wifi_owd = row
                elif row['name'] == 'lte::dl::owd':
                    df_lte_owd = row
                elif row['name'] == 'nr::dl::owd':
                    df_nr_owd = row
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
        #print(df_y_loc)

        if self.config_json["enable_wandb"]:
            # if not empty and send to wanDB database
            self.wandb_log_buffer_append(self.df_to_dict(df_phy_wifi_max_rate))
    
            self.wandb_log_buffer_append(self.df_to_dict(df_phy_lte_max_rate))

            dict_rate = self.df_to_dict(df_rate)
            if df_rate is not None:
                dict_rate["sum_rate"] = sum(df_rate["value"])
            self.wandb_log_buffer_append(dict_rate)

            dict_wifi_rate = self.df_to_dict(df_wifi_rate)
            if df_wifi_rate is not None:
                dict_wifi_rate["sum_wifi_rate"] = sum(df_wifi_rate["value"])
            self.wandb_log_buffer_append(dict_wifi_rate)

            dict_lte_rate = self.df_to_dict(df_lte_rate)
            if df_lte_rate is not None:
                dict_lte_rate["sum_lte_rate"] = sum(df_lte_rate["value"])
            self.wandb_log_buffer_append(dict_lte_rate)

            dict_nr_rate = self.df_to_dict(df_nr_rate)
            if df_nr_rate is not None:
                dict_nr_rate["sum_nr_rate"] = sum(df_nr_rate["value"])
            self.wandb_log_buffer_append(dict_nr_rate)

            self.wandb_log_buffer_append(self.df_to_dict(df_wifi_traffic_ratio)) 
            self.wandb_log_buffer_append(self.df_to_dict(df_lte_traffic_ratio)) 
            self.wandb_log_buffer_append(self.df_to_dict(df_nr_traffic_ratio)) 

            self.wandb_log_buffer_append(self.df_to_dict(df_wifi_owd)) 
            self.wandb_log_buffer_append(self.df_to_dict(df_lte_owd)) 
            self.wandb_log_buffer_append(self.df_to_dict(df_nr_owd)) 

            self.wandb_log_buffer_append(self.df_to_dict(df_x_loc))

            self.wandb_log_buffer_append(self.df_to_dict(df_y_loc))
        
        observation = np.vstack([lte_max_rate_value, wifi_max_rate_value, rate_value])
        # print('Observation --> ' + str(observation))
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
        # normalize the action for each user, such that the sum of splitting ratio per user equals 1.
        for user_action in action:
            sumRatio = user_action.sum()
            if sumRatio == 0:
                user_action[0] = 1
            else:
                user_action[:] = user_action[:]/sumRatio
        policy1 = json.loads(self.action_data_format.to_json())
        policy1["name"] = "dl::split_weight"
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

        if self.config_json["rl_config"]["reward_type"] == "utility":
            self.reward = self.netowrk_util(ave_rate, avg_delay)
        else:
            sys.exit("[ERROR] reward type not supported yet")

        if self.config_json["enable_wandb"]:
            self.wandb_log_buffer_append(self.df_to_dict(df_owd))
            self.wandb_log_buffer_append({"reward": self.reward, "avg_delay": avg_delay, "max_delay": max_delay})
        #self.render_network(df)

        return self.reward

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
    
    def render_network(self, df):
        if self.layout is None:
            return
        
        self.df = df

        left = self.layout["left"]
        mixin_left = Panel(self.plotextMixin(self.make_plot))
        left.update(mixin_left)


        self.layout["right"].split(
            Layout(name="right1", ratio = 1),
            Layout(name="right2", ratio = 1),
        )
        right1 = self.layout["right1"]
        table_right1 = self.make_table()
        right1.update(table_right1)

        right2 = self.layout["right2"]
        table_right2 = self.make_network_table()
        right2.update(table_right2)

        if self.layout["main"].visible == False:
            self.layout["main"].visible=True

    def make_plot(self, width, height):
        plt.clf()

        wifi_x = []
        wifi_y = []
        for loc in self.config_json["env_config"]["wifi_ap_locations"]:
            wifi_x.append(loc["x"])
            wifi_y.append(loc["y"])


        nr_x = []
        nr_y = []
        for loc in self.config_json["env_config"]["nr_gnb_locations"]:
            nr_x.append(loc["x"])
            nr_y.append(loc["y"])

        lte_x = [self.config_json["env_config"]["lte_enb_locations"]["x"]]
        lte_y = [self.config_json["env_config"]["lte_enb_locations"]["y"]]

        x_loc = None
        x_loc_id = None
        y_loc = None
        wifi_cell_id = None
        nr_cell_id = None
        lte_cell_id = None

        wifi_rate = None
        nr_rate = None
        lte_rate = None

        for index, row in self.df.iterrows():
            if row['source'] == 'gma':
                if row['name'] == 'x_loc':
                    x_loc = row['value']
                    x_loc_id = row['id']
                elif row['name'] == 'y_loc':
                    y_loc = row['value']
                elif row['name'] == 'wifi::dl::rate':
                    wifi_rate = row['value']
                elif row['name'] == 'nr::dl::rate':
                    nr_rate = row['value']
                elif row['name'] == 'lte::dl::rate':
                    lte_rate = row['value']
            elif row['source'] == 'wifi':
                if row['name'] == 'cell_id':
                    wifi_cell_id = [int(a) for a in row['value']]
            elif row['source'] == 'nr':
                if row['name'] == 'cell_id':
                   nr_cell_id = [int(a) for a in row['value']]
            elif row['source'] == 'lte':
                if row['name'] == 'cell_id':
                   lte_cell_id = [int(a) for a in row['value']]

        for idx, cell_id in enumerate(lte_cell_id):
            plot_color = "gray"
            if lte_rate[idx] > 0.0:
                plot_color = "red"
                plt.text(lte_rate[idx], 0.5*x_loc[idx]+0.5*lte_x[cell_id-1], 0.5*y_loc[idx]+0.5*lte_y[cell_id-1], alignment = 'center',  color=plot_color,  background=231)
            plt.plot([x_loc[idx], lte_x[cell_id-1]],[y_loc[idx], lte_y[cell_id-1]], marker = "braille", color=plot_color)
        if nr_cell_id is not None:
            for idx, cell_id in enumerate(nr_cell_id):
                plot_color = "gray"
                if nr_rate[idx] > 0.0:
                    plot_color = "green"
                    plt.text(nr_rate[idx], 0.5*x_loc[idx]+0.5*nr_x[cell_id-1], 0.5*y_loc[idx]+0.5*nr_y[cell_id-1], alignment = 'center',  color=plot_color, background=231)
                plt.plot([x_loc[idx], nr_x[cell_id-1]],[y_loc[idx], nr_y[cell_id-1]], marker = "braille", color=plot_color)
        if wifi_cell_id is not None:
            for idx, cell_id in enumerate(wifi_cell_id):
                plot_color = "gray"
                if wifi_rate[idx] > 0.0:
                    plot_color = "blue"
                    plt.text(wifi_rate[idx], 0.5*x_loc[idx]+0.5*wifi_x[cell_id-1], 0.5*y_loc[idx]+0.5*wifi_y[cell_id-1], alignment = 'center',  color=plot_color, background=231)
                plt.plot([x_loc[idx], wifi_x[cell_id-1]],[y_loc[idx], wifi_y[cell_id-1]], marker = "braille", color=plot_color)

        if x_loc is not None and y_loc is not None:
            #plt.scatter(x_loc, y_loc, marker = "u", color="white", label="user", style="inverted")
            [plt.text("ue-"+str(x_loc_id[i]), x = x_loc[i], y = y_loc[i], alignment = 'center', color = "black" ) for i in range(len(x_loc))]

        #plt.scatter(wifi_x, wifi_y, marker = "w", color="blue", label="wifi", style="inverted")
        [plt.text("wifi-"+str(i+1), x = wifi_x[i], y = wifi_y[i], alignment = 'center', color = 231, background = "blue") for i in range(len(wifi_x))]

        #plt.scatter(nr_x, nr_y, marker = "n", color="green", label="nr", style="inverted")
        [plt.text("nr-"+str(i+1), x = nr_x[i], y = nr_y[i], alignment = 'center', color = 231, background = "green") for i in range(len(nr_x))]

        #plt.scatter(lte_x, lte_y, marker = "l", color="red", label="lte", style="inverted")
        [plt.text("lte-"+str(i+1), x = lte_x[i], y = lte_y[i], alignment = 'center', color = 231, background = "red") for i in range(len(lte_x))]
        min_x = self.config_json["env_config"]["user_location_range"]["min_x"]
        max_x = self.config_json["env_config"]["user_location_range"]["max_x"]
        min_y = self.config_json["env_config"]["user_location_range"]["min_y"]
        max_y = self.config_json["env_config"]["user_location_range"]["max_y"]

        x_margin = (max_x-min_x)/20
        plt.text("gray link = 0.0 mbps", min_x-x_margin, max_y, alignment = 'left',  color="gray", background=231)

        # plot link
        plt.plotsize(width, height)
        plt.xaxes(1, 1)
        plt.yaxes(1, 1)
        plt.title("Network graph with link throughput (mbps)")
        plt.theme('clear')
        plt.ylim(min_y, max_y)
        plt.xlim(min_x-x_margin, max_x+x_margin)
        plt.ylabel("Y (m)")
        plt.xlabel("X (m)")
        return plt.build()

    def make_table(self):
        table = Table(title="Network configuration and stats")
        table.add_column("network")
        table.add_column("wifi", style="blue")
        table.add_column("nr", style="green")
        table.add_column("lte", style="red")
        table.add_column("multi-access", style="magenta")

        wifi_rate = None
        nr_rate = None
        lte_rate = None
        network_rate = None
        wifi_owd = None
        nr_owd = None
        lte_owd = None
        network_owd = None

        for index, row in self.df.iterrows():
            if row['source'] == 'gma':
                if row['name'] == 'wifi::dl::rate':
                    wifi_rate = row['value']
                elif row['name'] == 'nr::dl::rate':
                    nr_rate = row['value']
                elif row['name'] == 'lte::dl::rate':
                    lte_rate = row['value']
                elif row['name'] == 'dl::rate':
                    network_rate = row['value']
                elif row['name'] == 'wifi::dl::owd':
                    wifi_owd = row['value']
                elif row['name'] == 'nr::dl::owd':
                    nr_owd = row['value']
                elif row['name'] == 'lte::dl::owd':
                    lte_owd = row['value']
                elif row['name'] == 'dl::owd':
                    network_owd = row['value']


        wifi_rate_sum = .0
        if wifi_rate is not None:
            wifi_rate_sum = sum(wifi_rate)
        nr_rate_sum = 0
        if nr_rate is not None:
            nr_rate_sum = sum(nr_rate)
        lte_rate_sum = sum(lte_rate)
        network_rate_sum = sum(network_rate)
        table.add_row("frequency", "5.0GHz", "3.5GHz", "800MHz", "")

        lte_bw = self.config_json["env_config"]["lte"]["resource_block_num"]/5
        table.add_row("bandwith \u00D7 cell", "20MHz \u00D7 "+str(len(self.config_json["env_config"]["wifi_ap_locations"])), "20MHz \u00D7 "+str(len(self.config_json["env_config"]["nr_gnb_locations"])), "%.0f" %  (lte_bw)+"MHz(dl)/"+"%.0f" %  (lte_bw)+"MHz(ul) \u00D7 1", "")
    
        table.add_row("throughput (mbps)", "%.2f" % wifi_rate_sum, "%.2f" %  nr_rate_sum, "%.2f" %  lte_rate_sum, "%.2f" % network_rate_sum)

        wifi_owd_max = -1
        if wifi_owd is not None:
            wifi_owd_max = max(wifi_owd)
        nr_owd_max = -1
        if nr_owd is not None:
            nr_owd_max = max(nr_owd)
        lte_owd_max = max(lte_owd)
        network_owd_max = max(network_owd)
        table.add_row("max one-way delay (ms)", "%.0f" % wifi_owd_max, "%.0f" %  nr_owd_max, "%.0f" %  lte_owd_max, "%.0f" % network_owd_max)
        table.add_row("reward", "", "", "", "%.2f" % self.reward)
        panel = Panel(table)
        return panel
    
    def make_network_table(self):
        print(self.df)

        table1 = Table(title="wifi stats (per cell)")
        table1.add_column("wifi cell ID")

        for l in range(len(self.config_json["env_config"]["wifi_ap_locations"])):
            table1.add_column(str(l+1))
        

        table2 = Table(title="nr stats (per cell)")
        table2.add_column("nr cell ID")
        for l in range(len(self.config_json["env_config"]["nr_gnb_locations"])):
            table2.add_column(str(l+1))

        table3 = Table(title="lte stats (per cell)")
        table3.add_column("lte cell ID")
        table3.add_column("1")
        tabel_panel = []

        wifi_rate = None
        nr_rate = None
        lte_rate = None
        
        wifi_max_rate = None
        lte_max_rate = None

        wifi_cell_id = None
        nr_cell_id = None
        lte_cell_id = None
        for index, row in self.df.iterrows():
            if row['source'] == 'gma':
                if row['name'] == 'wifi::dl::rate':
                    wifi_rate = row['value']
                elif row['name'] == 'nr::dl::rate':
                    nr_rate = row['value']
                elif row['name'] == 'lte::dl::rate':
                    lte_rate = row['value']
                elif row['name'] == 'dl::rate':
                    network_rate = row['value']
                elif row['name'] == 'wifi::dl::owd':
                    wifi_owd = row['value']
                elif row['name'] == 'nr::dl::owd':
                    nr_owd = row['value']
                elif row['name'] == 'lte::dl::owd':
                    lte_owd = row['value']
                elif row['name'] == 'dl::owd':
                    network_owd = row['value']
            elif row['source'] == 'wifi':
                if row['name'] == 'dl::max_rate':
                    wifi_max_rate = row['value']
                if row['name'] == 'cell_id':
                    wifi_cell_id = [int(a) for a in row['value']]
            elif row['source'] == 'nr':
                if row['name'] == 'cell_id':
                    nr_cell_id = [int(a) for a in row['value']]
            elif row['source'] == 'lte':
                if row['name'] == 'dl::max_rate':
                    lte_max_rate = row['value']
                if row['name'] == 'cell_id':
                    lte_cell_id = [int(a) for a in row['value']]
                    
        #wifi
        if len(self.config_json["env_config"]["wifi_ap_locations"]) > 0:
            wifi_capacity_sum = np.zeros(len(self.config_json["env_config"]["wifi_ap_locations"]))
            wifi_counter = np.zeros(len(self.config_json["env_config"]["wifi_ap_locations"]))

            for c, r in zip(wifi_cell_id, wifi_max_rate):
                wifi_capacity_sum[c-1] += r
                wifi_counter[c-1] += 1
        
            
            input = ["est. capacity (mbps)"]
            for counter, capacity in zip(wifi_counter, wifi_capacity_sum):
                if counter > 0:
                    temp = capacity/counter
                    input.append("%.2f" %  temp)
                else:
                    input.append("")
            table1.add_row(*input)


            wifi_throughput_sum = np.zeros(len(self.config_json["env_config"]["wifi_ap_locations"]))
            wifi_counter = np.zeros(len(self.config_json["env_config"]["wifi_ap_locations"]))

            for c, r in zip(wifi_cell_id, wifi_rate):
                wifi_throughput_sum[c-1] += r
                wifi_counter[c-1] += 1

            input = ["throughput (mbps)"]
            for counter, tpt in zip(wifi_counter, wifi_throughput_sum):
                if counter > 0:
                    input.append("%.2f" %  tpt)
                else:
                    input.append("0")
            table1.add_row(*input)

            input = ["active users"]
            for counter in (wifi_counter):
                input.append(str(int(counter)))
            table1.add_row(*input)
            tabel_panel.append(table1)
        #nr
        if len(self.config_json["env_config"]["nr_gnb_locations"]) > 0:
            nr_capacity_sum = np.zeros(len(self.config_json["env_config"]["nr_gnb_locations"]))
            nr_counter = np.zeros(len(self.config_json["env_config"]["nr_gnb_locations"]))
        
            input = ["est. capacity (mbps)"]
            for counter, capacity in zip(nr_counter, nr_capacity_sum):
                if counter > 0:
                    temp = capacity/counter
                    input.append("%.2f" %  temp)
                else:
                    input.append("")
            table2.add_row(*input)
            
            nr_throughput_sum = np.zeros(len(self.config_json["env_config"]["nr_gnb_locations"]))
            nr_counter = np.zeros(len(self.config_json["env_config"]["nr_gnb_locations"]))

            for c, r in zip(nr_cell_id, nr_rate):
                nr_throughput_sum[c-1] += r
                nr_counter[c-1] += 1

            input = ["throughput (mbps)"]
            for counter, tpt in zip(nr_counter, nr_throughput_sum):
                if counter > 0:
                    input.append("%.2f" %  tpt)
                else:
                    input.append("0")
            table2.add_row(*input)

            input = ["active users"]
            for counter in (nr_counter):
                input.append(str(int(counter)))
            table2.add_row(*input)
            tabel_panel.append(table2)

        #lte
        lte_capacity_sum = np.zeros(1)
        lte_counter = np.zeros(1)

        for c, r in zip(lte_cell_id, lte_max_rate):
            lte_capacity_sum[c-1] += r
            lte_counter[c-1] += 1
      
        
        input = ["est. capacity (mbps)"]
        for counter, capacity in zip(lte_counter, lte_capacity_sum):
            if counter > 0:
                temp = capacity/counter
                input.append("%.2f" %  temp)
            else:
                input.append("")
        table3.add_row(*input)

        lte_throughput_sum = np.zeros(1)
        lte_counter = np.zeros(1)

        for c, r in zip(lte_cell_id, lte_rate):
            lte_throughput_sum[c-1] += r
            lte_counter[c-1] += 1

        input = ["throughput (mbps)"]
        for counter, tpt in zip(lte_counter, lte_throughput_sum):
            if counter > 0:
                input.append("%.2f" %  tpt)
            else:
                input.append("0")
        table3.add_row(*input)

        input = ["active users"]
        for counter in (lte_counter):
            input.append(str(int(counter)))
        table3.add_row(*input)
        tabel_panel.append(table3)

        panel = Panel(
            Columns(tabel_panel),
        )

        return panel