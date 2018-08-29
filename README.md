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

## Instructions - Linux

### Build

*Linux build has been verified on Ubuntu 16 and Ubuntu 18.*

#### Prerequisite

C++17 is required to build the Drive project.

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

Run the following command to create a test build on Ubuntu:

```
make
```

The binaries will be generated at `out/bin`. On Ubuntu, `.deb` installation packages are also automatically generated at `out/package`.

The difference between a test build and an official build is that the default service configration file is pointing to different addresses. The test build is starting services by using localhost as the kad directory service while the official build is using the current online service. **Test build is recommended for debugging purpose.**

To create an official build instead, run the following command:

```
make OFFICIAL_BUILD=1
```

To build on other Linux redistributions that does not support `.deb` installation packages, run the following build command to skip the generation for `.deb` packages:

```
make NO_DEB=1
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
`out/bin/bdfsclient`                | `/usr/local/bin`
`out/bin/drive`                     | `/usr/local/bin`
`src/bdfsclient/bdfsclientd`        | `/usr/local/bin`
`src/bdfsclient/bdfsclient.service` | `/etc/systemd/system`
`src/bdfs/bdfs.conf`                | `/etc/drive`

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

## Instructions (Windows)

TBD

## Instructions (macOS)

TBD
