cd /bin/
find -name "b*" -type f | xargs ls -l > ~/output
cd ~
awk '{print $9,$3,$1 > "output"}' output
awk -F "/" '{print $2 > "output"}' output
chmod 337 output
