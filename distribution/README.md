# Creating Binary installers of CCN-lite

For binary distibution of CCN-Lite one will find scripts here, to create installer packets for different platforms.
Requirement to create a binary installer is, to compile ccn-lite with NFN enabled (run 'export USE_NFN=1 && make clean all').
Usually it is required to switch to the Operating System, for which the installer should be created.
A binary installer depends on a platform. Thus it is not possible to use a amd64 installer on a i386 or arm system.


## Debian
Debian packets (*.deb) can be used to install software on Debian and its Derivatives.
To create Debian packets, the packet installer dpkg must be installed on the system.
Swap to the Debian folder and run 'sudo make'. The Debian file will be generated.


## Red Hat
RPM packets can be used to install software on Red Hat Enterprice Linux and its Derivatives.
To create RPM packets, the packet installer rpm and rpmbuild must be installed on the system.
Swap to the RetHat folder and run 'make'. The Debian file will be generated.
You will find the RPM file in rpmbuild/RPMS/ \< architecture \> /.
