# meta-rz-multi-os

This is a Multi-OS feature that provides support for the RZ/V2L of 64bit Arm-based MPU from Renesas Electronics. Currently the following board and MPU are supported:

- Board: RZV2L SMARC Evaluation Kit / MPU: R9A07G054L (RZ/V2L)

## Build Instructions

Assume that $WORK is the current working directory.

Download proprietary Multi-OS package from Renesas.
To download Multi-OS package, please use the following link:

    English: https://www.renesas.com/us/en/software-tool/rzv2l-group-multi-os-package
    Japanese: https://www.renesas.com/jp/ja/software-tool/rzv2l-group-multi-os-package

Please choose correct package that matches with your MPU.
After downloading the proprietary package, please decompress them then put meta-rz-features folder at $WORK.

- To apply Multi-OS feature, please run the command below:
```bash
   $ bitbake-layers add-layer ../meta-rz-features/meta-rz-multi-os/meta-<platform>
```
\<platform\> can be selected in below table:

|Renesas MPU| platform |
|:---------:|:--------:|
|RZ/V2L     |rzv2l     |