#!/bin/bash
mkdir -p logs_from_z6
echo "Pulling logs from z6..."
scp -r xx@192.168.0.165:~/ZetaZero/logs/* logs_from_z6/ 2>/dev/null || echo "No logs folder found or empty."
scp -r xx@192.168.0.165:~/ZetaZero/*.log logs_from_z6/ 2>/dev/null || echo "No log files found in root."
echo "Done. Logs are in logs_from_z6/"
ls -F logs_from_z6/
