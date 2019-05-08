# rooty
Academic project of Linux rootkit made for Bachelor Engineering Thesis.

More about project can be found in actual [thesis](https://github.com/jermeyyy/rooty/blob/master/docs/Praca%20In%C5%BCynierska%20-%20Karol%20Celebi.pdf) or in [article](https://github.com/jermeyyy/rooty/blob/master/docs/3_PT1-2_41-s39_CELEBI_SUSKI.pdf) written by Zbigniew Suski (thesis supervisor).

Whole rootkit is implemented as LKM module and few user-space services.

## Functionalities
- root access
- hiding itself
- control via IOCTL interface (client included)
- keylogger
- hide files/dirs
- hide processes
- hide tcp/udp IPv4/IPv6 connections
- remote root shell activated by magic ICMP packet
- VNC protocol service (screen preview only)

## Screenshots

### rooty LKM initialization
![](/art/init.png?raw=true)

### IOCTL control interface
![](/art/ioctl-control.png?raw=true)

### keylogger
![](/art/keylogger.png?raw=true)

### sshd initialization
![](/art/ssh-init.png?raw=true)

### sshd initialized
![](/art/ssh-init2.png?raw=true)

### remote access
![](/art/ssh-access.png?raw=true)

### vncd initialization
![](/art/vncd-init.png?raw=true)

### vncd running
![](/art/vncd-running.png?raw=true)
