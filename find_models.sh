#!/bin/bash
ssh xx@192.168.0.165 "find ~/ZetaZero -name '*.gguf' -maxdepth 4" > remote_models.txt
