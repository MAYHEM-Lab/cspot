curl -fsSL https://raw.githubusercontent.com/MAYHEM-Lab/cspot/caplets/dist/update-cspot-distribution.sh > update-cspot-distribution.sh
chmod 755 update-cspot-distribution.sh
./update-cspot-distribution.sh daily lib
if MUSL
	cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchain-musl.cmake
else
	cmake -S . -B build
endif
cd build
make
cd bin
cp ../../update-cspot-distribution.sh .
./update-cspot-distribution.sh daily
./cspot-app-example-init -W test -s 1000
./cspot-app-example-client -W test -S 100
