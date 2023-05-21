FROM ubuntu

# Set up container and time zones
RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ="America/New_York" apt-get -y install tzdata

# Download Linux support tools
RUN apt-get install -y \
    build-essential \
    wget \
    curl \
    openocd \
    git \
    gdb-multiarch \
    minicom

# Download setup support for Rasperry Pi Probe
RUN apt-get install -y \
    automake \
    autoconf \
    texinfo \
    libtool \
    libftdi-dev \
    libusb-1.0-0-dev \
    pkg-config

RUN git clone https://github.com/raspberrypi/openocd.git \
    --branch rp2040 \
    --depth=1 \
    --no-single-branch

# Build Rasberry Pi Probe Package
RUN cd openocd && ./bootstrap
RUN cd openocd && ./configure
RUN cd openocd && make -j4 && make install

# Set up a development tools directory
WORKDIR /home/dev
ADD . /home/dev

# Install cross compiler
RUN wget -qO- https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 | tar -xvj

#RUN apt install linux-tools-virtual hwdata
#RUN update-alternatives --install /usr/local/bin/usbip usbip \
#    `ls /usr/lib/linux-tools/*/usbip | tail -n1` 20

ENV PATH $PATH:/home/dev/gcc-arm-none-eabi-10.3-2021.10/bin

WORKDIR /home/app
