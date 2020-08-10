if [[ -n "$(command -v apt-get)" ]]; then
    sudo apt-get -y update
    sudo apt-get -y install libmlpack-dev
elif [[ -n "$(command -v yum)" ]]; then
    sudo yum -y update
    sudo yum -y install mlpack-devel centos-release-scl
    sudo yum -y install devtoolset-7
fi