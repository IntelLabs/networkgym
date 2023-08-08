#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_dummy_worker.py

from custom_env import CustomEnv
import time
from NetworkGymSim.network_gym_sim import build_ns3
from NetworkGymSim.network_gym_sim import NetworkGymSim
def main():
    """main function"""
    #build_ns3(config=False, build=False)

    num_workers= 20
    for worker in range(num_workers):
        customEnv = CustomEnv('env'+str(worker), NetworkGymSim, ['nqos_split', 'qos_steer', 'network_slicing'])
        customEnv.start()
        time.sleep(1)

if __name__ == "__main__":
    main()