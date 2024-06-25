# Meta-rz-features
This is a Yocto build layer that provides features of Renesas.

### Meta-rz-codecs
This layer provides (H.264/H.265) Video CODEC library for Renesas RZ/G2 and RZ/G2L Series.

### Meta-rz-graphics
This layer provides Graphics (OpenGL ES) library for Renesas RZ/G2 and RZ/G2L Series.

### Meta-renesas-ai
This layer provides AI tools support to the Renesas RZ/G2 and RZ/G2L Series.\
URI: https://github.com/renesas-rz/meta-renesas-ai

## Download instruction
To download all submodules, follow instruction as below:

Setup git configuration:
``` bash
git config --global user.email "you@example.com"
git config --glocal user.name "Your Name"
```

Assume that `$WORK` is your current working directory:
``` bash
cd $WORK
# Download and checkout appropriate commit for each submodule
git submodule update --init

# Update lastest source of submodules
git submodule update --recursive --remote
git pull --recurse-submodules
```
