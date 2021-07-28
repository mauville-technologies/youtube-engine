# Youtube Game Engine
Vulkan based Game Engine under development on YouTube channel Ozzadar. Meant for learning purposes -- not production quality (yet?).

## x11 dependencies
Personally tested on Ubuntu 20.04 with instructions provided for this. I expect adapting these to other distros shouldn't be too difficult.

### common tools
```bash
sudo apt install build-essential
```
### vulkan-sdk
```bash
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.182-focal.list https://packages.lunarg.com/vulkan/1.2.182/lunarg-vulkan-1.2.182-focal.list
sudo apt update
sudo apt install vulkan-sdk
```

### x11 dependencies
```bash
sudo apt install xorg-dev
```