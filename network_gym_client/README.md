 We currently host two networkgym versions in **external** vlab servers: a stable version updated less frequently (accessible via port 8088) and a development version with more frequent updates for new features (accessible via port 8092).
  - To switch to the stable version branch, use the following command: `git checkout mlwins-v01-8088`.
  - To switch to the development version branch, use the following command: `git checkout mlwins-v01-8092`.

First, request access to the [vLab](https://registration.intel-research.net/) machine.

## Setup Port Forwarding to vLab Server

**Skip this section if you plan to deploy the client on the mlwins-v01 vlab server.** Otherwise, follow the following steps to set up port forwarding from you local machine to the stable version of vlab Server. **Change the port number from 8092 to 8088 to access stable version.**
- First, setup port forwarding from the local port 8092 to the mlwins-v01 external server port 8092 via the SSH gateway using the following command in a screen session, e.g., `screen -S port8092`.
``` 
ssh -L 8092:mlwins-v01.research.intel-research.net:8092 ssh.intel-research.net
```
- If the previous command does not work, add your user account before the `ssh.intel-research.net` as follows.
```
ssh -L 8092:mlwins-v01.research.intel-research.net:8092 [YOUR_USER_NAME]@ssh.intel-research.net
```
 - If the previous command also does not work, add the following instructions to your ssh configure file, replace **[YOUR_USER_NAME]** with your user name and update **[PATH_TO_SSH]** accordingly.
```
# COMMAND: ssh mlwins

Host gateway
  HostName ssh.intel-research.net
  User [YOUR_USER_NAME]
  Port 22
  IdentityFile /home/[PATH_TO_SSH]/.ssh/id_rsa

Host mlwins
  HostName mlwins-v01.research.intel-research.net
  User [YOUR_USER_NAME]
  Port 22
  IdentityFile /home/[PATH_TO_SSH]/.ssh/id_rsa
  ProxyJump gateway
  LocalForward 8092 localhost:8092
```