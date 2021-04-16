FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y wget git python3 python3-pip xorg-dev libglu1-mesa-dev gcc-10 g++-10 rsync libvulkan-dev glslang-tools libglfw3-dev
RUN python3 -m pip install --upgrade pip && python3 -m pip install meson ninja
RUN wget -q -O miniconda.sh https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
RUN bash miniconda.sh -b -p /miniconda

ENV PATH="/miniconda/bin:${PATH}"

RUN conda update -n base -c defaults conda
RUN conda install cmake -c conda-forge

RUN mkdir /app
RUN mkdir /copy

COPY .github/runner.sh /
RUN chmod +x /runner.sh
ENTRYPOINT ["/runner.sh"]
