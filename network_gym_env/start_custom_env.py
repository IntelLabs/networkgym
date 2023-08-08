#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_dummy_worker.py

from custom_env import CustomEnv
import time
from dummy_sim import DummySim as NetworkGymSim

def main():
    """main function"""
    #for i in range(2):
    #    customEnv = CustomEnv('customEnv' + str(i), ['custom', 'custom_'+str(i)])
    #    customEnv.start()
    #    time.sleep(1)

    customEnv1 = CustomEnv('customEnv1', NetworkGymSim, ['common', 'custom'])
    customEnv1.start()
    time.sleep(1)
    customEnv2 = CustomEnv('customEnv2', NetworkGymSim, ['common', 'nqos_split', 'qos_steer', 'network_slicing'])
    customEnv2.start()

if __name__ == "__main__":
    main()