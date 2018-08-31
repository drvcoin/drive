# Drive

Drive is the storage platform that powers the future of decentralized computing.

Find more information about Drive at https://www.drvcoin.io.

## Components

The Drive project contains the following major components:

- Host: Host components that share/sell the space to other users. Hosts are supposed to be online all the time.
- Client: Client components that purchase and use the space from hosts.
- Kad: A directory service based on Kademlia DHT that allows clients to discover the available services that may meet its requirement.
- Relay (optional): A relay service that relays the network traffic when host is not public reachable or behind a firewall.

The Drive project is aimed to support different operating systems. The current status for the supported OS matrix is listed below:

Component | Linux | Windows | macOS
:-------: | :---: | :-----: | :---:
  Host    |   x   |         |      
  Client  |   x   |    x    |      
  Kad     |   x   |         |
  Relay   |   x   |         |

The detailed instructions for different platforms are covered in the following sections:

 - [Linux](#instructions-linux)
 - [Windows](#instructions-windows)
 - [macOS](#instructions-macos)

## Instructions - Linux

### Build

*Linux build has been verified on Ubuntu 16 and Ubuntu 18.*

#### Prerequisite

C++14 and cmake are required to build the Drive project.

The dependencies can be installed using the following command on Ubuntu:

```
sudo apt-get install libssl-dev libcurl4-openssl-dev uuid-dev
```

#### Build Steps

##### Clone the source code

The Drive repository contains a few submodules. Make sure all the modules are synced before starting a build.

*Relay is in seperate repositories and is disabled by default.*

Run the following command to sync to the latest code:

```
git submodule init
git submodule update
```

##### Create the build

Run the following commands to create a test build on Ubuntu:

```bash
cmake .
make
```

The binaries will be generated at `out/bin`. On Ubuntu, `.deb` installation packages are also automatically generated at `out/package`.

The difference between a test build and an official build is that the default service configration file is pointing to different addresses. The test build is starting services by using localhost as the kad directory service while the official build is using the current online service. **Test build is recommended for debugging purpose.**

To create an official build instead, run the following command:

```bash
cmake -DOFFICIAL_BUILD=1 .
make
```

To build on other Linux redistributions that does not support `.deb` installation packages, run the following build command to skip the generation for `.deb` packages:

```bash
cmake -DNO_DEB=1 .
make
```

### Run Drive

#### Kad Directory Service

*Kad directory service is already deployed online by Drive Foundation when using official build. It is not required to deploy your own Kad service.*

**Note: Kad service requires at least 2 nodes at this moment. Public accessible IP addresses or port forwarding is require in order to make the service accessible from hosts and clients.**

To deploy the kad service, copy `out/package/drive-bdkad_$version_$platform.deb` to the nodes, and run the following command to install:

```
sudo dpkg -i drive-bdkad_*.deb
```

If there is any missing dependency required, run the following command to fix that:

```
sudo apt-get install -f
```

Kad service is automatically started after installation.

Kad service uses `/etc/drive/bdkad.conf` as the configuration file which specifies the public IP address and port that is available for this node. If the node is behind a firewall and requires a different address or port, modify this file with the IP address and port to use. For test builds which uses its own local Kad services, also make sure the `node_name` of the first deployed node in this configuration file is set to `root`.

Once the configuration file is modified, restart the bdkad service to make it take effect by:

```
sudo service bdkad restart
```

For test builds, it is also required to modify the IP addres and port in `/etc/drive/default_contacts.json` on other kad nodes so that all the local kad services use the first kad node "`root`" as the initial contacts.

#### Host Service

To deploy the host service, copy `out/package/drive-bdhost_$version_$platform.deb` to the host nodes, and run the following command to install:

```
sudo dpkg -i drive-bdhost_*.deb
```

If there is any missing dependency required, run the following command to fix that:

```
sudo apt-get install -f
```

Kad service is automatically started using the default configration after installation.

The file `/etc/drive/bdhost.conf` contains the configurations that can be changed for host service. The following variables are some interesting ones to use the host service:

- PORT: the port host service is supposed on listen on.
- ENDPOINT: the URL of the host service. **Modify this value if different IP address or port is required, e.g. NAT.**
- KADEMLIA: the URL of the kad HTTP service. **Change this value to local Kad service for test builds.**
- SIZE: the total available size to publish. Default value is 1GB. The service will use `/var/drive` as the storage folder.

Make sure the host service is restarted after changing the configuration file by:

```
sudo service bdhost stop
sudo service bdhost start
```

#### Client

*Client installation package has not been completed yet. The content in this section is subject to change when the installation package is supported.*

1\. Copy the following files to the client machine.

File                                | Client Location
----------------------------------- | ---------------------
out/bin/bdfsclient                  | /usr/local/bin
out/bin/drive                       | /usr/local/bin
src/bdfsclient/bdfsclientd          | /usr/local/bin
src/bdfsclient/bdfsclient.service   | /etc/systemd/system
src/bdfs/bdfs.conf                  | /etc/drive

2\. Modify `/etc/drive/bdfs.conf` on client.

- kademlia: The kad HTTP service URL. Change it to local service for test builds.
- codeBlocks: Recommended to use 4 code blocks.
- dataBlocks: Recommended to use 4 data blocks.
- size: The default volume size the client would like to use when creating volumes without specifying a particular size.

3\. Start the client service.

```
sudo service bdfsclient start
```

4\. Use the client `drive` command to create, format and mount volumes. The following shows some sample command line usage:

```
# create a volume "test" with 1GB
drive create -n test -s 1073741824

# format the volume as ext4
drive format -n test ext4

# mount the volume to folder "mnt"
drive mount -n test mnt

# unmount the volume
drive unmount -n test

# delete the volume
drive delete -n test
```

To see more usage of the arguments from the command line, run:

```
drive --help
```

## Instructions - Windows

### Build

*Windows currently supports client components only. The build has been verified on 64-bit Windows 7 and Windows 10.*

#### Prerequisite

- Visual Studio 2017 Community or Visual Studio 2017 Express for Desktop
- Git

#### Build Steps

##### Clone the source code

The Drive repository contains a few submodules. Make sure all the modules are synced before starting a build.

Windows client is using branch `bdfs-win` instead of `master`. Make sure to run the following command to checkout to `bdfs-win` branch and sync to the latest code once the repository has been cloned.

```
git checkout bdfs-win
git submodule init
git submodule update
```

#### Create the build

1\. Open "Developer Command Prompt for VS 2017" from Start menu.

2\. Call `cd` in the command line window to change the directory to the `src` folder in the repository.

3\. Run the following command to create the build. The binaries will be created in folder `x64\Release`.

```
msbuild /p:Configuration=Release /p:Platform=x64 drive.sln
```

### Run Drive Windows Client

*The following steps to run drive client on Windows are subject to change once an installation package is implemented for Windows.*

1\. Install [Imdisk](http://www.ltr-data.se/files/imdiskinst_2.0.9.exe) device driver. Imdisk is an open source project that simulates block devices like hard drives on Windows. It allows the actual blocks to be provided from a user-mode process. The device driver installation package can be downloaded from http://www.ltr-data.se/files/imdiskinst_2.0.9.exe.

2\. Create a folder to contain all the executable binaries, including:

- `drive.exe` and `bdfsclient.exe` from the build output folder `x64\Release`.
- `libeay32.dll` and `ssleay32.dll` from folder `libs\openssl\bin`.

3\. Create folder `drive` in "`%appdata%`" and copy "`src\bdfs\bdfs.conf`" to this new created folder.

*The `bdfs.conf` can be modified to use a different Kad directory service URL for test build. Change the field "`kademlia`" to the test build Kad service URL to use your own Kad directory service.*

4\. Open an elevated "Command Line Prompt" window from Start menu. If UAC is enabled, make sure to open the window by right click the menu and choose "Run as Administrator".

5\. Run the following command to start the daemon process from the folder created in step 2.

```
bdfsclient
```

6\. Open another elevated "Command Line Prompt" window like step 4.

7\. Run the `drive` command to create or mount the volume. The following shows some samples to use the `drive` command:

- Create a volume "test" with 1GB

    ```
    drive create -n test -s 1073741824
    ```

- Mount the volume "test" to drive letter "Z:"

    ```
    drive mount -n test Z:
    ```

    *Windows explorer will prompt to format the disk when mounting the volume for the first time. The drive can be formatted with any file systems like NTFS or FAT32.*

- Unmount the volume "test"

    ```
    drive unmount -n test
    ```

## Instructions - macOS

TBD
