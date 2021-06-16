# Python extension for CSPOT

## Build dependency
```
cd ..
make python
```

## Run hello-world
```
cd python-ext/apps/hello-world/
make
cd cspot/
./woofc-namespace-platform &> log &
python start.py
cat log 
```

