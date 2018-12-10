# NuWriter command tool for NUC980 family processors
The Nu-writer Command Tool is a linux console application consisting of functions 
to access storage(eg. DRAM,NAND,SPINOR,SPINAND,SD) in a NUC980 family processors

## NuWriter command tool Installation Steps
debian/ubuntu : 
```
sudo apt-get install libusb-1.0-0-dev
```

compiler :
```
./configure --prefix=$PWD/install
make
make install
```
## NuWriter command tool examples
```
./nuwriter run.ini
```
Note: Refer to the "run.ini" in the source code.

**Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved**


