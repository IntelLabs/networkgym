#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_dummy_worker.py

from custom_env import CustomEnv
import time
from NetworkGymSim.network_gym_sim import build_ns3
from NetworkGymSim.network_gym_sim import NetworkGymSim
def main():
    """main function"""
    build_ns3(config=False, build=True)
    customEnv1 = CustomEnv('env1', NetworkGymSim, ['nqos_split', 'qos_steer', 'network_slicing'])
    customEnv1.start()
    #time.sleep(1)
    #customEnv2 = CustomEnv('customEnv2', NetworkGymSim,  ['common', 'nqos_split', 'qos_steer', 'network_slicing'])
    #customEnv2.start()

if __name__ == "__main__":
    main()