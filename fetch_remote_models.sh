#!/bin/bash
ssh xx@192.168.0.165 "find ~/ZetaZero -name '*.gguf' -maxdepth 5 > ~/models_list.txt"
scp xx@192.168.0.165:~/models_list.txt ./remote_models_list.txt
