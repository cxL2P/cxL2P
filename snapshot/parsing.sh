cat part_* > snapshot.tar.gz
tar -xzvf snapshot.tar.gz
./parsing_tool memory cxl2p nvme0n1_l2p_file
