
# CCN-Lite Docker Tutorial
This tutorial explains how ccn-lite as well as nfn-scala can be run in a Docker container. This keeps your system clean
and makes it possible to easily run several nodes on fully isolated system processes.

## Docker Installation
Just follow the official [installation](https://docs.docker.com/installation/#installation). For OSX you will have to install
Boot2Docker which runs a prebuild Docker image on virtual box.

## Building and running the CCN-Lite Docker Image
First you have to build the Docker container. A container is based on a certain image, in this case Ubuntu. From this base image
several commands are run to compile CCN-Lite and setup some variables and expose the port of CCN-Lite
(9000 instead of the default ports, because this makes it possible to use the same port for different protocols).
In Ubuntu you will have to provide sudo privileges by using `sudo docker ...` instead of `docker`.
```bash
cd $CCNL_HOME
docker build -t yourname/ccn-lite:devel .  (do not forget to put the (.) at the end)
```
With the -t flag you can give your container a name, which is usually your username from dockerhub (or any other name if you do not use dockerhub), the name of the container and a version flag at the end.

```bash
docker run -p 9000:9000/udp --name ccnl yourname/ccn-lite:devel 
```
The previous command runs the container, connects the internal port to the external port (`-p`) and
gives the running container a named handle (otherwise a random name is chosen).
Since there are no additional arguments after the container name, the default command is run (`CMD` statement of the Dockerfile).
If you want to run a different command in the container, you can start it with the following instead:
```bash
docker run -p 9000:9000/udp --name ccnl yourname/ccn-lite:devel /var/ccn-lite/bin/ccn-nfn-relay -d test/ndntlv -s ndn2013 -v 99 -u 9000
```
Now you should be able to send CCN requests to the container by using your locally installed ccn-lite-peek utility.
On Ubuntu, you can send the requests to 127.0.0.1. To send on OSX you have to get the IP with `boot2docker ip` (that is the address of the virtual machine running on VirtualBox).
```bash
$CCNL_HOME/bin/ccn-lite-peek -s ndn2013 -u x.x.x.x/9000 "/ndn/simple" | $CCNL_HOME/bin/ccn-lite-pktdump
```

To stop the container, use:
```bash
docker stop ccnl && docker rm ccnl
```

If something goes wrong, here some debugging tips:
* `docker ps` lists all running (and with `-a` you also see the stopped ones) containers
* `docker exec -i -t ccnl /bin/bash` starts a shell inside the container
* `docker logs ccnl` connects to the output of the initially run command, here the output of CCN-Lite

## nfn-scala Docker container
Very similar to CCN-Lite you can also start nfn-scala in a container.
By Default, this will start both a NFN enabled CCN-Lite relay as well as a Scala compute server.
Make sure that nfn-scala was checked out with `--recursive` (if you are not sure go to the `ccn-lite-nfn` and see if it is empty or not).
If you have not use `git submodule update --init --recursive` to get the correct CCN-Lite version as a submodule.
Here are the commands:
```bash
cd nfn-scala
docker build -t yourname/nfn-scala:devel .
docker run -p 9000:9000/udp --name nfnscala yourname/nfn-scala:devel
```
Only when done or something went wrong
```bash
docker stop nfnscala && docker rm nfnscala
```

Interests are still sent to the CCN-Lite port. We do not expose the port of nfn-scala because it is usually not accessed directly.
For the first request, the compute server is not involved because there is no call expression
```bash
$CCNL_HOME/bin/ccn-lite-peek -s ndn2013 -u x.x.x.x/9000 "" "add 1 2" | $CCNL_HOME/bin/ccn-lite-pktdump
```
The request will internally send a request to the compute server. The result will be a content object with the content '5'.
```bash
$CCNL_HOME/bin/ccn-lite-peek -s ndn2013 -u x.x.x.x/9000 "" "call 2 /docker/nfn/nfn_service_WordCount 'foo bar 1 2 3'" | $CCNL_HOME/bin/ccn-lite-pktdump
```
The result of the next computation does not fit into a single content object. Because the NFN-machinery does not support chunked results (and it is also questionable if this would make sense),
the result is a redirect. Neither NDN nor CCNx has a native mechanism supporting redirects, therefore we use a primitive protocol for redirects.
If the result is a redirect, the content starts with `redirect:` following the redirected name which has to be looked up manually, e.g. `redirect:/some/name`.
```bash
$CCNL_HOME/bin/ccn-lite-peek -s ndn2013 -u x.x.x.x/9000 "" "call 4 /docker/nfn/nfn_service_Pandoc /node/docker/docs/tutorial_md 'markdown_github' 'html'" | $CCNL_HOME/bin/ccn-lite-pktdump -f2
```
To get the actual result, use the `ccn-lite-fetch` utility with the redirected name (without the `redirect:`)
```bash
$CCNL_HOME/bin/ccn-lite-fetch -s ndn2013 -u x.x.x.x/9000 "/docker/nfn/call 4 %2fdocker%2fnfn%2fnfn_service_Pandoc %2fdocker%2fnfn%2fdocs%2ftutorial_md 'markdown_github' 'html'"
```
