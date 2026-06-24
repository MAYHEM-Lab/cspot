FROM ubuntu:22.04

# For building a docker image for building cspot.
# Build cspot by calling ./build.sh

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /
RUN mkdir -p code/src
RUN mkdir -p code/build
RUN apt update
RUN apt install git libzmq5-dev -y
RUN apt install libczmq-dev ninja-build g++-9 cmake g++ libssl-dev libyaml-dev -y

WORKDIR /code

ENTRYPOINT ["./build.sh","INSIDE_DOCKER_BUILD"]

# For debug, change entrypoint to: ["/bin/bash", "-c"], rerun docker build, and run 
# sudo docker run --rm -v "$PWD:/code" -it cspot-compiler /bin/bash