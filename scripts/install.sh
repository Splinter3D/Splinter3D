CURRENT_PATH=$(pwd)
rm -rf vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
cd $CURRENT_PATH