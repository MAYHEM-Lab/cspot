./pull.sh
python ../cspot/apps/dht/tools/parse.py dht9/log_host > find_result 
python ../cspot/apps/dht/tools/find_diff.py find_result dht9/log_cspot dht9/log_run