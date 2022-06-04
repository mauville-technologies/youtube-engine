# Youtube Game Engine
Vulkan based Game Engine under development on YouTube channel Ozzadar. Meant for learning purposes -- not production quality (yet?).

## x11 dependencies
Personally tested on Ubuntu 20.04 with instructions provided for this. I expect adapting these to other distros shouldn't be too difficult.

### common tools
```bash
    sudo apt-get install build-essential git make cmake autoconf automake \
    libtool pkg-config libasound2-dev libpulse-dev libaudio-dev libjack-dev \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
    libxss-dev libgl1-mesa-dev libdbus-1-dev \
    libudev-dev libgles2-mesa-dev libegl1-mesa-dev libibus-1.0-dev \
    fcitx-libs-dev libsamplerate0-dev libsndio-dev libwayland-dev \
    libxkbcommon-dev libdrm-dev libgbm-dev libxinerama-dev
```

### vulkan-sdk
```bash
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.182-focal.list https://packages.lunarg.com/vulkan/1.2.182/lunarg-vulkan-1.2.182-focal.list
sudo apt update
sudo apt install vulkan-sdk
```
