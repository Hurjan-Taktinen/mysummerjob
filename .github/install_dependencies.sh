#!/bin/sh
sudo apt-get update
sudo apt-get install -y wget \
    python3 \
    python3-pip \
    xorg-dev \
    libglu1-mesa-dev \
    gcc-10 \
    g++-10 \
    libvulkan-dev \
    glslang-tools \
    libglfw3-dev

# PIP shit
python3 -m pip install --upgrade pip
python3 -m pip install meson ninja

# Getting cmake 3.19 is hard
wget -q -O miniconda.sh https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
bash miniconda.sh -b -p $HOME/miniconda
export PATH="$HOME/miniconda/bin:${PATH}"
conda update -n base -c defaults conda
conda install cmake -c conda-forge
