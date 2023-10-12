FROM ubuntu:latest

# Set up container and time zones
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive TZ="America/New_York" \
    apt-get -y install tzdata

# Download Linux support tools
RUN apt-get install -y \
    build-essential \
    wget \
    curl \
    openocd \
    git \
    gdb-multiarch \
    minicom \
    vim

# Download setup support for Rasperry Pi Probe
RUN apt-get install -y \
    automake \
    autoconf \
    texinfo \
    libtool \
    libftdi-dev \
    libusb-1.0-0-dev \
    pkg-config

RUN git clone https://github.com/STMicroelectronics/openocd.git \
    --branch openocd-cubeide-r6 \
    --depth=1 \
    --no-single-branch

# Build Rasberry Pi Probe Package
RUN cd openocd && ./bootstrap
RUN cd openocd && ./configure
RUN cd openocd && make -j4 && make install

RUN wget https://builds.renode.io/renode-1.14.0+20231003gitf86ac3cf.linux-portable.tar.gz
RUN mkdir renode_portable && tar -xvf renode-*.linux-portable.tar.gz -C renode_portable --strip-components=1
ENV PATH $PATH:/renode_portable

# Set up a development tools directory
WORKDIR /home/dev
ADD . /home/dev

RUN echo 'if [ $n -e /home/app/shepherd.ioc ]; then echo \
" ______     __  __     ______     ______   __  __     ______     ______     _____\n\
/\  ___\   /\ \_\ \   /\  ___\   /\  == \ /\ \_\ \   /\  ___\   /\  == \   /\  __-.\n\
\ \___  \  \ \  __ \  \ \  __\   \ \  _-/ \ \  __ \  \ \  __\   \ \  __<   \ \ \/\ \ \n\
 \/\_____\  \ \_\ \_\  \ \_____\  \ \_\    \ \_\ \_\  \ \_____\  \ \_\ \_\  \ \____- \n\ 
  \/_____/   \/_/\/_/   \/_____/   \/_/     \/_/\/_/   \/_____/   \/_/ /_/   \/____/"; fi;' >> ~/.bashrc

RUN echo 'if [ $n -e /home/app/cerberus.ioc ]; then echo \
"_________             ___.               \n\
\_   ___ \  __________\_ |__   ___________ __ __  ______\n\
/    \  \/_/ __ \_  __ \ __ \_/ __ \_  __ \  |  \/  ___/\n\
\     \___\  ___/|  | \/ \_\ \  ___/|  | \/  |  /\___ \ \n\
 \______  /\___  >__|  |___  /\___  >__|  |____//____  >\n\
        \/     \/          \/     \/                 \/ "; fi;' >> ~/.bashrc

RUN echo 'alias open_serial="minicom -b 115200 -o -D /dev/ttyACM0"' >> ~/.bashrc
RUN echo 'alias flash_stm="openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c \"adapter speed 5000\" -c \"program ./build/cerberus.elf verify reset exit\""' >> ~/.bashrc

# Install cross compiler
RUN wget -qO- https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 | tar -xvj

ENV PATH $PATH:/home/dev/gcc-arm-none-eabi-10.3-2021.10/bin

WORKDIR /home/app
