#!/bin/bash
scp remote_run_zeta.sh xx@192.168.0.165:~/ZetaZero/llama.cpp/
ssh xx@192.168.0.165 "chmod +x ~/ZetaZero/llama.cpp/remote_run_zeta.sh && ~/ZetaZero/llama.cpp/remote_run_zeta.sh"
