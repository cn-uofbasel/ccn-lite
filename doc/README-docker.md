# CCN-lite for Docker

This document contains information on the usage of
[Docker](https://www.docker.com/) with CCN-lite. Additionally, it provides
guidelines on how to build your own CCN-lite Docker container.

Running a CCN-lite relay inside a Docker container can help keeping your system
clean and makes it possible to run several nodes on fully isolated system
processes.

The [CCN-lite repository on Docker Hub](https://registry.hub.docker.com/u/cnuofbasel/ccn-lite/) contains a number of tags corresponding to different branches/tags:

tag name | GitHub branch/tag
:------- | :----------------
`latest` | [`master`](https://github.com/cn-uofbasel/ccn-lite/tree/master)
`dev`    | [`dev-master`](https://github.com/cn-uofbasel/ccn-lite/tree/dev-master)
`0.3.0`  | [`0.3.0`](https://github.com/cn-uofbasel/ccn-lite/releases/tag/0.3.0)
`0.2.0`  | [`0.2.0`](https://github.com/cn-uofbasel/ccn-lite/releases/tag/0.2.0)


## Prerequisites

CCN-lite for Docker obviously requires a working Docker installation. For more
information, visit the
[installation instructions](https://docs.docker.com/installation/) of Docker.



## Installation

1.  Choose a tag of the CCN-lite Docker image (see above) by defining a variable
    `$img`. To use the newest release, use tag `0.3.0`:

    ```bash
    img="cnuofbasel/ccn-lite:0.3.0"
    ```

    To use the newest development version, use `dev`:

    ```bash
    img="cnuofbasel/ccn-lite:dev"
    ```

2.  Define additional environment variables and aliases:

    ```bash
    UNAME_OS=$(uname -s 2> /dev/null || echo not)
    if [ "$UNAME_OS" = "Linux" ]; then sudo="sudo"; else sudo=""; fi

    alias drun="$sudo docker run"
    ```

    `$sudo` enables the conditional usage of `sudo` on Linux machines without
    enabling it on OS X. `drun` is an alias to run Docker containers.

3.  Pull the Docker image from [Docker Hub](https://registry.hub.docker.com/u/cnuofbasel/ccn-lite/):

    ```bash
    $sudo docker pull $img
    ```

4.  Run the container:

    ```bash
    drun -p 9000:9000/udp --name ccnl $img
    ```

    Option `-p` connects the internal Docker port to the external port of the
    host machine. If you need multiple ports exposed, specify `-p` multiple
    times.

    `--name` assigns the running container a named handle; without the flag a
    random name is chosen. A given name makes it easier to access the container
    and stop/remove it.

    To run the container in the background, add option `-d`.

    Without any additional arguments, Docker runs the default command specified
    by `CMD` in the Dockerfile: A CCN-lite NFN relay on port `9000` with suite
    `ndn2013` and logging set to `trace`. To run a custom command (for example
    using another suite), pass the command line arguments after the container
    name:

    ```bash
    drun -p 9000:9000/udp --name ccnl $img ccn-nfn-relay -v trace -u 9000 -d test/cistlv -s cisco2015
    ```

5.  Test if the container is running correctly by requesting a content object:

    ```bash
    HOST_IP=$(ifconfig docker0 | sed -n 's/.*inet addr:\(.*\) Bcast.*/\1/p')
    drun $img ccn-lite-peek -s ndn2013 -u $HOST_IP/9000 /ndn/simple | drun -i $img ccn-lite-pktdump
    ```

    Notice that the Docker container of `ccn-lite-peek` connects to port 9000 of the host. All Docker containers are run in an isolated network environment and only connected to the host via the virtual network device `docker0`. In order to access the host from inside a container, we need to obtain the host's IP address, listed in `docker0` (`$HOST_IP`).

    To redirect the output of `ccn-lite-peek` to `ccn-lite-pktdump`, we need to pass option `-i`. `-i` enables an interactive mode so that `ccn-lite-pktdump` can read from the host's `stdin`.

6.  Stop the container (`stop`) and remove it (`rm`). This might take some seconds.

    ```bash
    $sudo docker stop ccnl && $sudo docker rm ccnl
    ```



## Usage

The Dockerfile is set up in such a way that all CCN-lite command line utilities are directly accessible. For example you can create a content object using `ccn-lite-mkC`. `ccn-lite-mkC` reads from `stdin`, so type something and then press `Return`:

```bash
drun -i $img ccn-lite-mkC -s ndn2013 /ndn/mycontent > mycontent.ndtlv
```

You can verify the content object by passing it to `ccn-lite-pktdump`:

```bash
drun -i $img ccn-lite-pktdump < mycontent.ndtlv
```



## Debugging

In order to debug the container, the following commands might prove useful:

-  `$sudo docker ps` lists the running containers. Add option `-a` to see all
   containers.

-  `$sudo docker logs ccnl` fetches the output of the CCN-lite container `ccnl`.

-  `$sudo docker exec -i -t ccnl /bin/bash` starts a Bash shell *inside* the
   container `ccnl`.

-  `drun -i -t --name ccnl $img /bin/bash` starts a Bash shell in a *new* container `ccnl`.



## Building the Docker image by hand

In order to build your own CCN-lite Docker image, follow the [UNIX installation instructions](README-unix.md) to set up the CCN-lite sources (in particular [`Dockerfile`](../Dockerfile)) and relevant environment variables.

Then, use `docker build` (*do not forget to put the `.` at the end*):

```bash
cd $CCNL_HOME
$sudo docker build -t "<your-name>/ccn-lite:dev" .
```

`<your-name>` usually refers to your Docker Hub username. If you are not using Docker Hub, you can choose a name freely (for
example your user account name).
