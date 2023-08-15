#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_custom-env.py

from network_gym_env import CustomEnv
import time
from network_gym_env import DummySim as NetworkGymSim

def main():
    """main function"""

    customEnv1 = CustomEnv('custom_env_1', NetworkGymSim, ['custom'], 8087)
    customEnv1.start()
    time.sleep(1)
    customEnv2 = CustomEnv('custom_env_2', NetworkGymSim, ['custom'], 8087)
    customEnv2.start()

if __name__ == "__main__":
    main()