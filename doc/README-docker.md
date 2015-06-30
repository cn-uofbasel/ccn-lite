# CCN-lite for Docker

This readme explains how [CCN-lite](https://github.com/cn-uofbasel/ccn-lite) can
be run in a [Docker](https://www.docker.com/) container. This keeps your system
clean and makes it possible to easily run several nodes on fully isolated system
processes.

## Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up the
CCN-lite sources (in particular [`Dockerfile`](../Dockerfile)) and relevant
environment variables.

CCN-lite for Docker obviously requires a working Docker installation. For more
information, visit the [installation instructions](https://docs.docker.com/installation/)
of Docker.

## Installation

1.  Define environment variables:

    -  `$DOCKER_NAME` that refers to your username from
       [Docker Hub](https://hub.docker.com/):

       ```bash
       DOCKER_NAME=<your Docker Hub username>
       ```

       If you are not using Docker Hub, you can choose a name freely, for
       example your user account name:

       ```bash
       DOCKER_NAME=$(whoami)
       ```

    -  `$SUDO` to enable usage of `sudo` on Linux machines but disable it on OS X:

       ```bash
       UNAME_OS=$(uname -s 2>/dev/null || echo not)
       if [ "$UNAME_OS" = "Linux" ]; then SUDO="sudo"; else SUDO=""; fi
      ```

2.  Build the Docker container. *Do not forget to put the `.` at the end.*

    ```bash
    cd $CCNL_HOME
    $SUDO docker build -t "$DOCKER_NAME/ccn-lite:devel" .
    ```

    With option `-t` you can specify a name for the container which consists of
    the Docker Hub username, the container name and the version.

3.  Run the container:

    ```bash
    $SUDO docker run -p 9000:9000/udp --name ccnl "$DOCKER_NAME/ccn-lite:devel"
    ```

    Option `-p` connects the internal to the external port: `9000 --> 9000`.
    `--name` assigns the running container a named handle, without the flag a
    random name is chosen. To run the container in the background, add option `-d`.

    Without any additional arguments, Docker runs the default command specified
    by `CMD` in the Dockerfile. To run a custom command (for example using
    another suite), pass the command line arguments after the container name:

    ```bash
    $SUDO docker run -p 9000:9000/udp --name ccnl "$DOCKER_NAME/ccn-lite:devel" \
    /var/ccn-lite/bin/ccn-nfn-relay -d test/cistlv -s cisco2015 -v trace -u 9000
    ```

4.  Test if the container is running correctly by requesting a content object:

    ```bash
    DOCKER_IP=$(boot2docker ip 2> /dev/null || { echo 127.0.0.1 })
    $CCNL_HOME/bin/ccn-lite-peek -s ndn2013 -u "$DOCKER_IP/9000" "/ndn/simple" \
    | $CCNL_HOME/bin/ccn-lite-pktdump
    ```

    Notice that because Docker relies on Linux-specific features, it cannot be
    run natively on OS X. Thus, `$DOCKER_IP` is either the address returned by
    `boot2docker ip` (OS X) or `127.0.0.1` (Linux) if it does not exist.

5.  Stop the container with (this might take some seconds):

    ```bash
    $SUDO docker stop ccnl && $SUDO docker rm ccnl
    ```


## Debugging

In order to debug the container, the following commands might prove useful:

-  `$SUDO docker ps` lists the running containers. Add option `-a` to see all
   containers.

-  `$SUDO docker exec -i -t ccnl /bin/bash` starts a Bash shell *inside* the
   container.

-  `$SUDO docker logs ccnl` fetches the output of the CCN-lite container.
