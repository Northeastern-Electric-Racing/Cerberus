FROM ubuntu

# Set up container and time zones
RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ="America/New_York" apt-get -y install tzdata

# Download Linux support tools
RUN apt-get install -y \
    build-essential \
    wget \
    curl

# Set up a development tools directory
WORKDIR /home/dev
ADD . /home/dev

# Install cross compiler
RUN wget -qO- https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 | tar -xj

ENV PATH $PATH:/home/dev/gcc-arm-none-eabi-10.3-2021.10/bin

WORKDIR /home/app
