


#cd build 
#make -j && 
make install
ldconfig
cd ../python

sudo rm -fr dist
sudo python3 setup.py bdist_wheel
sudo pip3 uninstall -y ngt
sudo pip3 install dist/ngt-*-linux_x86_64.whl
