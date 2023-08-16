#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_custom-env.py

from network_gym_env import Configure
import time
from network_gym_env import DummySim as NetworkGymSim

def main():
    """main function"""

    customEnv1 = Configure(1, NetworkGymSim)
    customEnv1.start()
    time.sleep(1)
    customEnv2 = Configure(2, NetworkGymSim)
    customEnv2.start()

if __name__ == "__main__":
    main()