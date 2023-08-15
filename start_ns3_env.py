#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_ns_env.py

from network_gym_env import CustomEnv
import time
from NetworkGymSim.network_gym_sim import build_ns3
from NetworkGymSim.network_gym_sim import NetworkGymSim
def main():
    """main function"""
    build_ns3(config=True, build=True)

    num_workers= 1
    for worker in range(num_workers):
        customEnv = CustomEnv('intel_ns3_'+str(worker), NetworkGymSim, ['nqos_split', 'qos_steer', 'network_slicing'], 8087)
        customEnv.start()
        time.sleep(1)

if __name__ == "__main__":
    main()