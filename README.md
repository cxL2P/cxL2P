# cxL2P
The source code for **cxL2P**: *"Augmenting High-Capacity SSD with a CXL-Backed Extendible Hybrid Index."*

## Directory
- **`cxl2p/`**: Source code for cxL2P.
- **`snapshot/`**: Real SSD L2P mapping snapshot.

## Parsing L2P Snapshot
To get the L2P sequence length distribution in Section 3.1, we showcase a sample L2P snapshot of our private cloud. Please run the following commands to parse it:
```bash
cd snapshot/
./parsing.sh
```
## Environment Requirement
- At least 20 cores and 96GB DRAM in the physical machine to enable seamless run of the following default FEMU scripts emulating a 32GB SSD in a VM with  vCPUs and 4GB DRAM.

## How to run in FEMU
```
1.Download and Install FEMU.
  Download FEMU and set up the environment following the official instructions.
2.Replace Files in FEMU:
  Copy all files from the cxl2p/ directory to the following FEMU path: hw/femu/bbssd/. This will replace the existing ftl.c and ftl.h files in the directory.
3.Configure Parameters:
  Edit the run-blackbox.sh script and adjust the parameters according to the configurations described in the cxL2P paper, and modify the `hw/femu/meson.build` file to inclue the new adding files.
4.Start FEMU:
  Run FEMU with the configured setup to execute cxL2P.
```
