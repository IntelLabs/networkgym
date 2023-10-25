#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_ns_env.py

from network_gym_env import Configure
import time
from network_gym_sim.network_gym_sim import build_ns3
from network_gym_sim.network_gym_sim import NetworkGymSim
def main():
    """main function"""
    build_ns3(config=True, build=True)

    num_workers= 3
    for worker in range(num_workers):
        customEnv = Configure(worker, NetworkGymSim, ['nqos_split', 'qos_steer', 'network_slicing', 'rmcat'])
        customEnv.start()
        time.sleep(0.1)

if __name__ == "__main__":
    main()